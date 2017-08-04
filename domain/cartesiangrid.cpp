#include <QTextStream>
#include <QFile>
#include "cartesiangrid.h"
#include "../util.h"
#include "attribute.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "geostats/gridcell.h"

CartesianGrid::CartesianGrid( QString path )  : DataFile( path )
{
    this->_dx = 0.0;
    this->_dy = 0.0;
    this->_dz = 0.0;
    this->_no_data_value = "";
    this->_nreal = 0;
    this->_nx = 0;
    this->_ny = 0;
    this->_nz = 0;
    this->_rot = 0.0;
    this->_x0 = 0.0;
    this->_y0 = 0.0;
    this->_z0 = 0.0;
}

void CartesianGrid::setInfo(double x0, double y0, double z0,
                            double dx, double dy, double dz,
                            int nx, int ny, int nz, double rot, int nreal, const QString no_data_value,
                            QMap<uint, QPair<uint, QString> > nvar_var_trn_triads,
                            const QList< QPair<uint,QString> > &categorical_attributes)
{
    //updating metadata
    this->_dx = dx;
    this->_dy = dy;
    this->_dz = dz;
    this->_no_data_value = no_data_value;
    this->_nreal = nreal;
    this->_nx = nx;
    this->_ny = ny;
    this->_nz = nz;
    this->_rot = rot;
    this->_x0 = x0;
    this->_y0 = y0;
    this->_z0 = z0;
    _nsvar_var_trn.clear();
    _nsvar_var_trn.unite( nvar_var_trn_triads );
    _categorical_attributes.clear();
    _categorical_attributes << categorical_attributes;

    this->updatePropertyCollection();
}

void CartesianGrid::setInfoFromMetadataFile()
{
    QString md_file_path( this->_path );
    QFile md_file( md_file_path.append(".md") );
    double x0 = 0.0, y0 = 0.0, z0 = 0.0;
    double dx = 0.0, dy = 0.0, dz = 0.0;
    uint nx = 0, ny = 0, nz = 0;
    double rot = 0.0;
    uint nreal = 0;
    QString ndv;
    QMap<uint, QPair<uint,QString> > nsvar_var_trn;
    QList< QPair<uint,QString> > categorical_attributes;
    if( md_file.exists() ){
        md_file.open( QFile::ReadOnly | QFile::Text );
        QTextStream in(&md_file);
        for (int i = 0; !in.atEnd(); ++i)
        {
           QString line = in.readLine();
           if( line.startsWith( "X0:" ) ){
               QString value = line.split(":")[1];
               x0 = value.toDouble();
           }else if( line.startsWith( "Y0:" ) ){
               QString value = line.split(":")[1];
               y0 = value.toDouble();
           }else if( line.startsWith( "Z0:" ) ){
               QString value = line.split(":")[1];
               z0 = value.toDouble();
           }else if( line.startsWith( "NX:" ) ){
               QString value = line.split(":")[1];
               nx = value.toInt();
           }else if( line.startsWith( "NY:" ) ){
               QString value = line.split(":")[1];
               ny = value.toInt();
           }else if( line.startsWith( "NZ:" ) ){
               QString value = line.split(":")[1];
               nz = value.toInt();
           }else if( line.startsWith( "DX:" ) ){
               QString value = line.split(":")[1];
               dx = value.toDouble();
           }else if( line.startsWith( "DY:" ) ){
               QString value = line.split(":")[1];
               dy = value.toDouble();
           }else if( line.startsWith( "DZ:" ) ){
               QString value = line.split(":")[1];
               dz = value.toDouble();
           }else if( line.startsWith( "ROT:" ) ){
               QString value = line.split(":")[1];
               rot = value.toDouble();
           }else if( line.startsWith( "NREAL:" ) ){
               QString value = line.split(":")[1];
               nreal = value.toInt();
           }else if( line.startsWith( "NDV:" ) ){
               QString value = line.split(":")[1];
               ndv = value;
           }else if( line.startsWith( "NSCORE:" ) ){
               QString triad = line.split(":")[1];
               QString var = triad.split(">")[0];
               QString pair = triad.split(">")[1];
               QString ns_var = pair.split("=")[0];
               QString trn_filename = pair.split("=")[1];
               //normal variable index is key
               //variable index and transform table filename are the value
               nsvar_var_trn.insert( ns_var.toUInt(), QPair<uint,QString>(var.toUInt(), trn_filename ));
           }else if( line.startsWith( "CATEGORICAL:" ) ){
               QString var_and_catDefName = line.split(":")[1];
               QString var = var_and_catDefName.split(",")[0];
               QString catDefName = var_and_catDefName.split(",")[1];
               categorical_attributes.append( QPair<uint,QString>(var.toUInt(), catDefName) );
           }
        }
        md_file.close();
        this->setInfo( x0, y0, z0, dx, dy, dz, nx, ny, nz, rot, nreal, ndv, nsvar_var_trn, categorical_attributes);
    }
}

void CartesianGrid::setInfoFromOtherCG(CartesianGrid *other_cg, bool copyCategoricalAttributesList)
{
    double x0 = 0.0, y0 = 0.0, z0 = 0.0;
    double dx = 0.0, dy = 0.0, dz = 0.0;
    uint nx = 0, ny = 0, nz = 0;
    double rot = 0.0;
    uint nreal = 0;
    QString ndv;
    QMap<uint, QPair<uint, QString> > nsvar_var_trn_triads;
    QList< QPair<uint, QString> > categorical_attributes;
    x0 = other_cg->getX0();
    y0 = other_cg->getY0();
    z0 = other_cg->getZ0();
    nx = other_cg->getNX();
    ny = other_cg->getNY();
    nz = other_cg->getNZ();
    dx = other_cg->getDX();
    dy = other_cg->getDY();
    dz = other_cg->getDZ();
    rot = other_cg->getRot();
    nreal = other_cg->getNReal();
    ndv = other_cg->getNoDataValue();
    nsvar_var_trn_triads = other_cg->getNSVarVarTrnTriads();
    if( copyCategoricalAttributesList )
        categorical_attributes = other_cg->getCategoricalAttributes();
    this->setInfo( x0, y0, z0, dx, dy, dz, nx, ny, nz, rot, nreal,
                   ndv, nsvar_var_trn_triads, categorical_attributes);
}

void CartesianGrid::setInfoFromGridParameter(GSLibParGrid *pg)
{
    double x0 = 0.0, y0 = 0.0, z0 = 0.0;
    double dx = 0.0, dy = 0.0, dz = 0.0;
    uint nx = 0, ny = 0, nz = 0;
    double rot = 0.0;
    uint nreal = 0;
    QString ndv;
    QMap<uint, QPair<uint, QString> > empty;
    QList< QPair<uint,QString> > empty2;

    nx = pg->_specs_x->getParameter<GSLibParUInt*>(0)->_value; //nx
    x0 = pg->_specs_x->getParameter<GSLibParDouble*>(1)->_value; //min x
    dx = pg->_specs_x->getParameter<GSLibParDouble*>(2)->_value; //cell size x
    ny = pg->_specs_y->getParameter<GSLibParUInt*>(0)->_value; //ny
    y0 = pg->_specs_y->getParameter<GSLibParDouble*>(1)->_value; //min y
    dy = pg->_specs_y->getParameter<GSLibParDouble*>(2)->_value; //cell size y
    nz = pg->_specs_z->getParameter<GSLibParUInt*>(0)->_value; //nz
    z0 = pg->_specs_z->getParameter<GSLibParDouble*>(1)->_value; //min z
    dz = pg->_specs_z->getParameter<GSLibParDouble*>(2)->_value; //cell size z

    rot = 0.0;
    nreal = 1;
    ndv = "";

    this->setInfo( x0, y0, z0, dx, dy, dz, nx, ny, nz, rot, nreal, ndv, empty, empty2);
}

void CartesianGrid::setCellGeometry(int nx, int ny, int nz, double dx, double dy, double dz)
{
    this->setInfo( _x0, _y0, _z0, dx, dy, dz, nx, ny, nz, _rot, _nreal,
                   _no_data_value, _nsvar_var_trn, _categorical_attributes);
}

std::vector<std::complex<double> > CartesianGrid::getArray(int indexColumRealPart, int indexColumImaginaryPart)
{
    std::vector< std::complex<double> > result( _nx * _ny * _nz ); //[_nx][_ny][_nz]

    for( uint k = 0; k < _nz; ++k)
        for( uint j = 0; j < _ny; ++j)
            for( uint i = 0; i < _nx; ++i){
                double real = 0.0d;
                double im = 0.0d;
                if( indexColumRealPart >= 0 )
                    real = dataIJK( indexColumRealPart, i, j, k);
                if( indexColumImaginaryPart >= 0 )
                    im = dataIJK( indexColumImaginaryPart, i, j, k);
                result[i + j*_nx + k*_ny*_nx] = std::complex<double>(real, im);
            }

    return result;
}

std::vector<std::vector<double> > CartesianGrid::getResampledValues(int rateI, int rateJ, int rateK,
                                                                    int &finalNI, int &finalNJ, int &finalNK)
{
    std::vector< std::vector<double> > result;

    uint totDataLinesPerRealization = _nx * _ny * _nz;

    //load just the first line to get the number of columns (assuming the file is right)
    setDataPage( 0, 0 );
    uint nDataColumns = getDataColumnCount();

    result.reserve( _nreal * (_nx/rateI) * (_ny/rateJ) * (_nz/rateK) * nDataColumns );

    //for each realization (at least one)
    for( uint r = 0; r < _nreal; ++r ){
        //compute the first and last data lines to load (does not need to load everything at once)
        ulong firstDataLine = r * totDataLinesPerRealization;
        ulong lastDataLine = firstDataLine + totDataLinesPerRealization - 1;
        //load the data corresponding to a realization
        setDataPage( firstDataLine, lastDataLine );
        loadData();
        //perform the resampling for a realization
        finalNK = 0;
        for( uint k = 0; k < _nz; k += rateK, ++finalNK ){
            finalNJ = 0;
            for( uint j = 0; j < _ny; j += rateJ, ++finalNJ ){
                finalNI = 0;
                for( uint i = 0; i < _nx; i += rateI, ++finalNI ){
                    std::vector<double> dataLine;
                    dataLine.reserve( nDataColumns );
                    for( uint d = 0; d < nDataColumns; ++d){
                        dataLine.push_back( dataIJK( d, i, j, k ) );
                    }
                    result.push_back( dataLine );
                }
            }
        }
    }

    //TODO: remove this when all GammaRay features become realization-aware.
    setDataPageToAll();

    return result;
}

double CartesianGrid::valueAt(uint dataColumn, double x, double y, double z)
{
    //TODO: add support for rotations
    if( ! Util::almostEqual2sComplement( this->_rot, 0.0, 1) ){
        Application::instance()->logError("CartesianGrid::valueAt(): rotation not supported yet.  Returning NDV or NaN.");
        if( this->hasNoDataValue() )
            return getNoDataValueAsDouble();
        else
            return std::numeric_limits<double>::quiet_NaN();
    }

    //compute the indexes from the spatial location.
    double xWest = _x0 - _dx/2.0;
    double ySouth = _y0 - _dy/2.0;
    double zBottom = _z0 - _dz/2.0;
    int i = (x - xWest) / _dx;
    int j = (y - ySouth) / _dy;
    int k = 0;
    if( _nz > 1 )
        k = (z - zBottom) / _dz;

    //check whether the location is outside the grid
    if( i < 0 || i >= (int)_nx || j < 0 || j >= (int)_ny || k < 0 || k >= (int)_nz ){
        //returns NDV if it is defined, NaN otherwise
        if( this->hasNoDataValue() )
            return getNoDataValueAsDouble();
        else
            return std::numeric_limits<double>::quiet_NaN();
    }

    //get the value
    return this->dataIJK( dataColumn, i, j, k);
}

void CartesianGrid::setDataPageToRealization(uint nreal)
{
    if( nreal >= _nreal ){
        Application::instance()->logError("CartesianGrid::setDataPageToRealization(): invalid realization number: " +
                                          QString::number( nreal ) + " (max. == " + QString::number( _nreal ) + "). Nothing done.");
        return;
    }
    ulong firstLine = nreal * _nx * _ny * _nz;
    ulong lastLine = (nreal+1) * _nx * _ny * _nz - 1; //the interval in DataFile::setDataPage() is inclusive.
    setDataPage( firstLine, lastLine );
}

double CartesianGrid::getDiagonalLength()
{
    //TODO: add support for rotations
    if( ! Util::almostEqual2sComplement( this->_rot, 0.0, 1) ){
        Application::instance()->logError("CartesianGrid::getDiagonalLength(): rotation not supported yet.  Returning NDV or NaN.");
        if( this->hasNoDataValue() )
            return getNoDataValueAsDouble();
        else
            return std::numeric_limits<double>::quiet_NaN();
    }
    double x0 = _x0 - _dx / 2.0d;
    double y0 = _y0 - _dy / 2.0d;
    double z0 = _z0 - _dz / 2.0d;
    double xf = x0 + _dx * _nx;
    double yf = y0 + _dy * _ny;
    double zf = z0 + _dz * _nz;
    double dx = xf - x0;
    double dy = yf - y0;
    double dz = zf - z0;
    return std::sqrt<double>( dx*dx + dy*dy + dz*dz ).real();
}

SpatialLocation CartesianGrid::getCenter()
{
    SpatialLocation result;
    //TODO: add support for rotations
    if( ! Util::almostEqual2sComplement( this->_rot, 0.0, 1) ){
        Application::instance()->logError("CartesianGrid::getCenter(): rotation not supported yet.  Returning NDV or NaN.");
        double errorValue;
        if( this->hasNoDataValue() )
            errorValue = getNoDataValueAsDouble();
        else
            errorValue = std::numeric_limits<double>::quiet_NaN();
        result._x = errorValue;
        result._y = errorValue;
        result._z = errorValue;
        return result;
    }
    double x0 = _x0 - _dx / 2.0d;
    double y0 = _y0 - _dy / 2.0d;
    double z0 = _z0 - _dz / 2.0d;
    double xf = x0 + _dx * _nx;
    double yf = y0 + _dy * _ny;
    double zf = z0 + _dz * _nz;
    result._x = (xf - x0) / 2.0;
    result._y = (yf - y0) / 2.0;
    result._z = (zf - z0) / 2.0;
    return result;
}

bool CartesianGrid::canHaveMetaData()
{
    return true;
}

QString CartesianGrid::getFileType()
{
    return "CARTESIANGRID";
}

void CartesianGrid::updateMetaDataFile()
{
    QFile file( this->getMetaDataFilePath() );
    file.open( QFile::WriteOnly | QFile::Text );
    QTextStream out(&file);
    out << APP_NAME << " metadata file.  This file is generated automatically.  Do not edit this file.\n";
    out << "version=" << APP_VERSION << '\n';
    out << "X0:" << QString::number( this->_x0, 'g', 12) << '\n';
    out << "Y0:" << QString::number( this->_y0, 'g', 12) << '\n';
    out << "Z0:" << QString::number( this->_z0, 'g', 12) << '\n';
    out << "NX:" << this->_nx << '\n';
    out << "NY:" << this->_ny << '\n';
    out << "NZ:" << this->_nz << '\n';
    out << "DX:" << this->_dx << '\n';
    out << "DY:" << this->_dy << '\n';
    out << "DZ:" << this->_dz << '\n';
    out << "ROT:" << this->_rot << '\n';
    out << "NREAL:" << this->_nreal << '\n';
    out << "NDV:" << this->_no_data_value << '\n';
    QMapIterator<uint, QPair<uint,QString> > j( this->_nsvar_var_trn );
    while (j.hasNext()) {
        j.next();
        out << "NSCORE:" << j.value().first << '>' << j.key() << '=' << j.value().second << '\n';
    }
    QList< QPair<uint,QString> >::iterator k = _categorical_attributes.begin();
    for(; k != _categorical_attributes.end(); ++k){
        out << "CATEGORICAL:" << (*k).first << "," << (*k).second << '\n';
    }
    file.close();
}

QIcon CartesianGrid::getIcon()
{
    if(_nz == 1){
        if( Util::getDisplayResolutionClass() == DisplayResolution::NORMAL_DPI )
            return QIcon(":icons/cartesiangrid16");
        else
            return QIcon(":icons32/cartesiangrid32");
    }else{
        if( Util::getDisplayResolutionClass() == DisplayResolution::NORMAL_DPI )
            return QIcon(":icons/cg3D16");
        else
            return QIcon(":icons32/cg3D32");
    }
}

void CartesianGrid::save(QTextStream *txt_stream)
{
    (*txt_stream) << this->getFileType() << ":" << this->getFileName() << '\n';
    //also saves the metadata file.
    this->updateMetaDataFile();
}

View3DViewData CartesianGrid::build3DViewObjects(View3DWidget *widget3D)
{
    return View3DBuilders::build( this, widget3D );
}
