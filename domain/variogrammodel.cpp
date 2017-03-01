#include "variogrammodel.h"

#include <QTextStream>

#include "gslib/gslibparameterfiles/gslibparameterfile.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "util.h"

VariogramModel::VariogramModel(const QString path) : File( path )
{
}

double VariogramModel::getSill()
{
    readParameters();
    return m_Sill;
}

double VariogramModel::getNugget()
{
    readParameters();
    return m_Nugget;
}

uint VariogramModel::getNst()
{
    readParameters();
    return m_nst;
}

uint VariogramModel::getIt(int structure)
{
    readParameters();
    return m_it.at( structure );
}

double VariogramModel::getCC(int structure)
{
    readParameters();
    return m_cc.at( structure );
}

double VariogramModel::get_a_hMax(int structure)
{
    readParameters();
    return m_a_hMax.at( structure );
}

double VariogramModel::get_a_hMin( int structure )
{
    readParameters();
    return m_a_hMin.at( structure );
}

double VariogramModel::get_a_vert(int structure)
{
    readParameters();
    return m_a_vert.at( structure );
}

double VariogramModel::getAzimuth(int structure)
{
    readParameters();
    return m_Azimuth.at( structure );
}

double VariogramModel::getDip(int structure)
{
    readParameters();
    return m_Dip.at( structure );
}

double VariogramModel::getRoll(int structure)
{
    readParameters();
    return m_Roll.at( structure );
}


QIcon VariogramModel::getIcon()
{
    if( Util::getDisplayResolutionClass() == DisplayResolution::NORMAL_DPI )
        return QIcon(":icons/vmodel16");
    else
        return QIcon(":icons32/vmodel32");
}

void VariogramModel::save(QTextStream *txt_stream)
{
    (*txt_stream) << this->getFileType() << ":" << this->getFileName() << '\n';
}

void VariogramModel::readParameters()
{
    //reset the ranges and angles collections
    m_it.clear();
    m_cc.clear();
    m_a_hMax.clear();
    m_a_hMin.clear();
    m_a_vert.clear();
    m_Azimuth.clear();
    m_Dip.clear();
    m_Roll.clear();

    //Variogram models are stored as parameter files for the vmodel GSLib program,
    //ergo, to read the values, just create a parameter file object for vmodel
    GSLibParameterFile par_vmodel( "vmodel" );
    par_vmodel.setValuesFromParFile( getPath() );

    //get the nugget effect
    GSLibParMultiValuedFixed *par3 = par_vmodel.getParameter<GSLibParMultiValuedFixed*>(3);
    m_Nugget = par3->getParameter<GSLibParDouble*>(1)->_value;

    //Sill starts with the nugget effect value.
    m_Sill = m_Nugget;

    //get the number of variogram structures
    GSLibParRepeat *par4 = par_vmodel.getParameter<GSLibParRepeat*>(4); //repeat nst-times
    m_nst = par3->getParameter<GSLibParUInt*>(0)->_value;
    par4->setCount( m_nst );

    //for each structure...
    for( uint inst = 0; inst < m_nst; ++inst)
    {
        GSLibParMultiValuedFixed *par4_0 = par4->getParameter<GSLibParMultiValuedFixed*>(inst, 0);
        //...collect the struture type
        m_it.append( par4_0->getParameter<GSLibParOption*>(0)->_selected_value );
        //...collect the contribution
        m_cc.append( par4_0->getParameter<GSLibParDouble*>(1)->_value );
        //...add the contribution to the Sill value
        m_Sill += par4_0->getParameter<GSLibParDouble*>(1)->_value;
        //...collect the angles
        m_Azimuth.append( par4_0->getParameter<GSLibParDouble*>(2)->_value );
        m_Dip.append( par4_0->getParameter<GSLibParDouble*>(3)->_value );
        m_Roll.append( par4_0->getParameter<GSLibParDouble*>(4)->_value );
        //...collect the ranges
        GSLibParMultiValuedFixed *par4_1 = par4->getParameter<GSLibParMultiValuedFixed*>(inst, 1);
        m_a_hMax.append( par4_1->getParameter<GSLibParDouble*>(0)->_value );
        m_a_hMin.append( par4_1->getParameter<GSLibParDouble*>(1)->_value );
        m_a_vert.append( par4_1->getParameter<GSLibParDouble*>(2)->_value );
    }
}