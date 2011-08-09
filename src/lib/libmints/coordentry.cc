#include <cmath>
#include <libmints/vector3.h>
#include <libmints/molecule.h>
#include <exception.h>
#include "coordentry.h"

using namespace psi;

double
VariableValue::compute()
{
    if(geometryVariables_.count(name_) == 0)
        throw PSIEXCEPTION("Variable " + name_ + " used in geometry specification has not been defined");
    return negate_ ? -geometryVariables_[name_] : geometryVariables_[name_];
}

CoordEntry::CoordEntry(int entry_number, int Z, double charge, double mass, const std::string& symbol, const std::string& label)
    : entry_number_(entry_number), computed_(false), Z_(Z),
      charge_(charge), mass_(mass), symbol_(symbol), label_(label), ghosted_(false)
{
}

CoordEntry::CoordEntry(int entry_number, int Z, double charge, double mass, const std::string& symbol,
           const std::string& label, const std::map<std::string, std::string>& basis)
    : entry_number_(entry_number), computed_(false), Z_(Z),
      charge_(charge), mass_(mass), symbol_(symbol), label_(label), ghosted_(false),
      basissets_(basis)
{

}

CoordEntry::~CoordEntry()
{

}

/**
 * Compares the charge, mass, ghost status, and basis set(s) of the current atom with those of another atom
 *
 * @return Whether the two atoms are the same.
 */
bool CoordEntry::is_equivalent_to(const boost::shared_ptr<CoordEntry> &other) const
{
    if(other->Z_ != Z_) return false;
    if(other->mass_ != mass_) return false;
    if(other->ghosted_ != ghosted_) return false;
    std::map<std::string, std::string>::const_iterator iter = basissets_.begin();
    std::map<std::string, std::string>::const_iterator stop = basissets_.end();
    for(; iter != stop; ++iter){
        std::map<std::string, std::string>::const_iterator other_it = other->basissets_.find(iter->first);
        if(other_it == other->basissets_.end()) return false; // This basis was never defined for the other atom
        if(iter->second != other_it->second) return false; // The basis sets are different
    }
    return true;
}

void CoordEntry::set_basisset(const std::string& name, const std::string& type)
{
    basissets_[type] = name;
}

const std::string& CoordEntry::basisset(const std::string& type) const
{
    std::map<std::string, std::string>::const_iterator iter = basissets_.find(type);

    if (iter == basissets_.end())
        throw PSIEXCEPTION("CoordEntry::basisset: Basisset not set for "+label_+" and type of " + type);

    return (*iter).second;
}


CartesianEntry::CartesianEntry(int entry_number, int Z, double charge, double mass, const std::string& symbol, const std::string& label,
                               boost::shared_ptr<CoordValue> x, boost::shared_ptr<CoordValue> y, boost::shared_ptr<CoordValue> z)
    : CoordEntry(entry_number, Z, charge, mass, symbol, label), x_(x), y_(y), z_(z)
{
}

CartesianEntry::CartesianEntry(int entry_number, int Z, double charge, double mass, const std::string& symbol, const std::string& label,
               boost::shared_ptr<CoordValue> x, boost::shared_ptr<CoordValue> y, boost::shared_ptr<CoordValue> z,
               const std::map<std::string, std::string>& basis)
    : CoordEntry(entry_number, Z, charge, mass, symbol, label, basis), x_(x), y_(y), z_(z)
{
}

const Vector3& CartesianEntry::compute()
{
    if(computed_)
        return coordinates_;

    coordinates_[0] = x_->compute();
    coordinates_[1] = y_->compute();
    coordinates_[2] = z_->compute();
    computed_ = true;
    return coordinates_;
}

void
CartesianEntry::set_coordinates(double x, double y, double z)
{
    coordinates_[0] = x;
    coordinates_[1] = y;
    coordinates_[2] = z;

    x_->set(x);
    y_->set(y);
    z_->set(z);

    computed_ = true;
}

ZMatrixEntry::ZMatrixEntry(int entry_number, int Z, double charge, double mass, const std::string& symbol, const std::string& label,
                           boost::shared_ptr<CoordEntry> rto, boost::shared_ptr<CoordValue> rval,
                           boost::shared_ptr<CoordEntry> ato, boost::shared_ptr<CoordValue> aval,
                           boost::shared_ptr<CoordEntry> dto, boost::shared_ptr<CoordValue> dval)
    : CoordEntry(entry_number, Z, charge, mass, symbol, label),
      rto_(rto), rval_(rval),
      ato_(ato), aval_(aval),
      dto_(dto), dval_(dval)
{
}

ZMatrixEntry::ZMatrixEntry(int entry_number, int Z, double charge, double mass, const std::string& symbol, const std::string& label,
             const std::map<std::string, std::string>& basis,
             boost::shared_ptr<CoordEntry> rto,
             boost::shared_ptr<CoordValue> rval,
             boost::shared_ptr<CoordEntry> ato,
             boost::shared_ptr<CoordValue> aval,
             boost::shared_ptr<CoordEntry> dto,
             boost::shared_ptr<CoordValue> dval)
    : CoordEntry(entry_number, Z, charge, mass, symbol, label, basis),
      rto_(rto), rval_(rval),
      ato_(ato), aval_(aval),
      dto_(dto), dval_(dval)
{
}

ZMatrixEntry::~ZMatrixEntry()
{
}

void
ZMatrixEntry::set_coordinates(double x, double y, double z)
{
    coordinates_[0] = x;
    coordinates_[1] = y;
    coordinates_[2] = z;

    if(rto_ != 0){
        if(!rto_->is_computed())
             throw PSIEXCEPTION("Coordinates have been set in the wrong order");
        rval_->set(r(coordinates_, rto_->compute()));
    }
    if(ato_ != 0){
        if(!ato_->is_computed())
             throw PSIEXCEPTION("Coordinates have been set in the wrong order");
        aval_->set(180.0*a(coordinates_, rto_->compute(), ato_->compute())/M_PI);
    }
    if(dto_ != 0){
        if(!dto_->is_computed())
             throw PSIEXCEPTION("Coordinates have been set in the wrong order");
        double val = d(coordinates_, rto_->compute(), ato_->compute(), dto_->compute());
        // Check for NaN, and don't update if we find one
        if(val == val)
            dval_->set(180.0*val/M_PI);
    }

    computed_ = true;
}


/**
 * Computes the coordinates of the current atom's entry
 * @Return The Cartesian Coordinates, in Bohr
 */
const Vector3& ZMatrixEntry::compute()
{
    if (computed_){
        return coordinates_;
}

    if(rto_ == 0 && ato_ == 0 && dto_ == 0){
        // The first atom
        coordinates_[0] = 0.0;
        coordinates_[1] = 0.0;
        coordinates_[2] = 0.0;
    }else if(ato_ == 0 && dto_ == 0){
        // The second atom
        coordinates_[0] = 0.0;
        coordinates_[1] = 0.0;
        coordinates_[2] = rval_->compute();
    }else if(dto_ == 0){
        // The third atom
        double r = rval_->compute();
        double a = aval_->compute() * M_PI/180.0;
        /*
         * The atom specification is
         *      this       rTo   rVal  aTo  aVal
         *        A         B           C
         * We arbitrarily chose to put A pointing upwards.
         */
        const Vector3& B = rto_->compute();
        const Vector3& C = ato_->compute();
        Vector3 eBC = C - B;
        eBC.normalize();
        double theta  = a - acos(eBC[2]); // The angle subtended by eBA and the vertical
        double rextra = r * sin(theta);   // How far atom A sticks out beyond the end of the CB line
        double phi = acos(eBC[0]);        // The angle subtended by eCB and the x axis
        coordinates_[0] = C[0] + rextra * cos(phi);
        coordinates_[1] = C[1] + rextra * sin(phi);
        coordinates_[2] = B[2] + r * cos(theta);
    }else{
        // The fourth, or subsequent, atom
        double r = rval_->compute();
        double a = aval_->compute() * M_PI/180.0;
        double d = dval_->compute() * M_PI/180.0;

        /*
         * The atom specification is
         *      this       rTo   rVal  aTo  aVal   dTo   dVal
         *        A         B           C           D
         * which allows us to define the vector from B->C (eBC) as the +z axis, and eAB
         * lies in the xz plane.  Then eX, eY and eZ (=eBC) are the x, y, and z axes, respecively.
         */
        const Vector3& B = rto_->compute();
        const Vector3& C = ato_->compute();
        const Vector3& D = dto_->compute();

        Vector3 eCD = C - D;
        Vector3 eBC = B - C;
        eCD.normalize();
        eBC.normalize();
        double cosBCD = cos(a);
        double sinBCD = sin(a);
        double cosABCD = cos(d);
        double sinABCD = sin(d);
//        double cosABC = -eAB.dot(eBC);
//        double sinABC = sqrt(1.0 - cosABC * cosABC);
//        if(sinABC < LINEAR_A_TOL || sinBCD < LINEAR_A_TOL)
//            throw PSIEXCEPTION("Atom defines a dihedral using a linear bond angle");
        Vector3 eY = eCD.perp_unit(eBC);
        Vector3 eX = eY.perp_unit(eBC);

        for(int xyz = 0; xyz < 3; ++xyz)
           coordinates_[xyz] = B[xyz] + r * ( -eBC[xyz] * cosBCD + eX[xyz] * sinBCD * cosABCD + eY[xyz] * sinBCD * sinABCD);
    }
    computed_ = true;
    return coordinates_;

}

