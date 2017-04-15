#include "softindicatorcalibrationdialog.h"
#include "ui_softindicatorcalibrationdialog.h"

#include "domain/application.h"
#include "domain/attribute.h"
#include "domain/file.h"
#include "domain/datafile.h"
#include "widgets/fileselectorwidget.h"
#include "softindicatorcalibplot.h"

#include <QHBoxLayout>

SoftIndicatorCalibrationDialog::SoftIndicatorCalibrationDialog(Attribute *at, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SoftIndicatorCalibrationDialog),
    m_at( at ),
    m_softIndCalibPlot( nullptr )
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    setWindowTitle("Soft indicator calibration for " + at->getContainingFile()->getName() + "/" + at->getName());

    //add a category definition file selection drop down menu
    m_fsw = new FileSelectorWidget( FileSelectorType::CategoryDefinitions, true );
    ui->frmTopBar->layout()->addWidget( m_fsw );

    //add a spacer for better layout
    QHBoxLayout *hl = (QHBoxLayout*)(ui->frmTopBar->layout());
    hl->addStretch();

    //add the widget used to edit the calibration curves
    m_softIndCalibPlot = new SoftIndicatorCalibPlot(this);
    ui->frmCalib->layout()->addWidget( m_softIndCalibPlot );

    //get the Attribute's data file
    File *file = m_at->getContainingFile();
    if( file->isDataFile() ){
        DataFile *dataFile = (DataFile*)file;
        //load the data
        dataFile->loadData();
        //create an array of doubles to store the Attribute's values
        std::vector<double> data;
        // get the total number of file records
        uint nData = dataFile->getDataLineCount();
        //pre-allocate the array of doubles
        data.reserve( nData );
        //get the Attribute's GEO-EAS index
        uint atGEOEASIndex = m_at->getAttributeGEOEASgivenIndex();
        //fills the array of doubles with the data from file
        for( uint i = 0; i < nData; ++i){
            double value = dataFile->data( i, atGEOEASIndex-1 );
            //adds only if the value is not a No-Data-Value
            if( ! dataFile->isNDV( value ) )
                data.push_back( value );
        }
        //move the array of doubles to the widget (it'll no longer be availabe here)
        m_softIndCalibPlot->transferData( data );
    }

    adjustSize();
}

SoftIndicatorCalibrationDialog::~SoftIndicatorCalibrationDialog()
{
    delete ui;
    Application::instance()->logInfo("SoftIndicatorCalibrationDialog destroyed.");
}
