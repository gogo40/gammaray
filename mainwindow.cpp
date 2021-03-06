#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QScreen>
#include "dialogs/aboutdialog.h"
#include "dialogs/setupdialog.h"
#include <QFileDialog>
#include "domain/project.h"
#include "domain/application.h"
#include <QtGlobal>
#include <QSettings>
#include "dialogs/datafiledialog.h"
#include "dialogs/pointsetdialog.h"
#include "dialogs/cartesiangriddialog.h"
#include "dialogs/creategriddialog.h"
#include "dialogs/krigingdialog.h"
#include "dialogs/triadseditordialog.h"
#include "dialogs/postikdialog.h"
#include "dialogs/ndvestimationdialog.h"
#include "domain/pointset.h"
#include "domain/cartesiangrid.h"
#include "domain/categorydefinition.h"
#include "domain/objectgroup.h"
#include "domain/univariatecategoryclassification.h"
#include <QModelIndex>
#include <QModelIndexList>
#include <typeinfo>
#include <cmath>
#include "domain/projectcomponent.h"
#include "domain/file.h"
#include "dialogs/filecontentsdialog.h"
#include "domain/attribute.h"
#include "dialogs/displayplotdialog.h"
#include "gslib/gslibparams/gslibparinputdata.h"
#include "gslib/gslibparameterfiles/gslibparameterfile.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "gslib/gslib.h"
#include "gslib/gslibparametersdialog.h"
#include "dialogs/variogramanalysisdialog.h"
#include "dialogs/declusteringdialog.h"
#include <QDesktopServices>
#include <QInputDialog>
#include <QLineEdit>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QTimer>
#include "domain/variogrammodel.h"
#include "domain/experimentalvariogram.h"
#include "domain/thresholdcdf.h"
#include "domain/categorypdf.h"
#include "util.h"
#include "dialogs/nscoredialog.h"
#include "dialogs/distributionmodelingdialog.h"
#include "dialogs/bidistributionmodelingdialog.h"
#include "dialogs/valuespairsdialog.h"
#include "dialogs/indicatorkrigingdialog.h"
#include "dialogs/gridresampledialog.h"
#include "spatialindex/spatialindexpoints.h"
#include "softindiccalib/softindicatorcalibrationdialog.h"
#include "dialogs/cokrigingdialog.h"
#include "dialogs/multivariogramdialog.h"
#include "dialogs/sgsimdialog.h"
#include "viewer3d/view3dwidget.h"
#include "imagejockey/imagejockeydialog.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_subMenuClassifyInto( new QMenu("Classify into", this) ),
    m_subMenuClassifyWith( new QMenu("Classify with", this) ),
    m_subMenuMapAs( new QMenu("Map as", this) )
{
    //Import any registry/home user settings of a previous version
    Util::importSettingsFromPreviousVersion();
    //arranges UI widgets.
    ui->setupUi(this);
    //puts application name and version in window title bar.
    this->setWindowTitle(APP_NAME_VER);
    //maximizes the window
    this->showMaximized();
    //retore main window splitter position
    ui->splitter->restoreState( Application::instance()->getMainWindowSplitterSetting() );
    //retore contents/message splitter position
    ui->splitter_2->restoreState( Application::instance()->getContentsMessageSplitterSetting() );
    //create menu actions associated with the most recently opened projects
    for (int i = 0; i < MaxRecentProjects; ++i) {
         _MRUactions[i] = new QAction(this);
         _MRUactions[i]->setVisible(false); //by default they are all hidden
         connect(_MRUactions[i], SIGNAL(triggered()), this, SLOT(openRecentProject()));
    }
    ui->menuFile->addSeparator();
    for (int i = 0; i < MaxRecentProjects; ++i)
        ui->menuFile->addAction(_MRUactions[i]);
    updateRecentProjectActions();
    //configure project tree context menu
    _projectContextMenu = new QMenu( ui->treeProject );
    ui->treeProject->setContextMenuPolicy( Qt::CustomContextMenu );
    connect(ui->treeProject, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onProjectContextMenu(const QPoint &)));

    //enable and configure the Project Tree's drag-and-drop feature.
    ui->treeProject->setDragEnabled(true);
    ui->treeProject->setDragDropMode(QAbstractItemView::DragDrop);
    ui->treeProject->viewport()->setAcceptDrops(true);
    ui->treeProject->setDropIndicatorShown(true);

    //configure project header context menu
    _projectHeaderContextMenu = new QMenu( ui->lblProjName );
    ui->lblProjName->setContextMenuPolicy( Qt::CustomContextMenu );
    connect(ui->lblProjName, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onProjectHeaderContextMenu(const QPoint &)));

    //update UI with application state.
    displayApplicationInfo();
    Application::instance()->setMainWindow( this );
    //open the lastly opened project if the user
    //closed GammaRay without closing the project
    if( ! Util::programWasCalledWithCommandLineArgument("-nolops") ){
        QString lops = Application::instance()->getLastlyOpenedProjectSetting();
        if( ! lops.isEmpty() )
            this->openProject( lops );
    }else{
        Application::instance()->logWarn("ATTENTION: Automatic project loading was disabled. (-nolops argument was passed via command line)");
    }
    //set project tree style
    this->refreshTreeStyle();
    //restore project tree state
    this->restoreProjectTreeUIState2();

    //show welcome message
    Application::instance()->logInfo(QString("Welcome to ").append(APP_NAME_VER).append("."));

    //set high-res icon if screen dpi is high
    if( Util::getDisplayResolutionClass() == DisplayResolution::HIGH_DPI ){
        this->setWindowIcon( QIcon(":icons32/logo64") );
    }

    //show screen DPI
    QScreen *screen0 = QApplication::screens().at(0);
    qreal rDPI = (qreal)screen0->logicalDotsPerInch();
    Application::instance()->logInfo(QString("----screen DPI (display 0): ").append(QString::number( rDPI )).append("."));

    //add a custom menu item to the QTextEdit's standard context menu.
    ui->txtedMessages->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->txtedMessages,SIGNAL(customContextMenuRequested(const QPoint&)),this,SLOT(showMessagesConsoleCustomContextMenu(const QPoint &)));

    //enable drop from drag-n-drop gestures
    setAcceptDrops( true );

    //show the 3D view widget (if user allowed it)
    if( ! Util::programWasCalledWithCommandLineArgument("-no3d") )
        ui->frmContent->layout()->addWidget( new View3DWidget( this ) );
    else
        Application::instance()->logWarn("ATTENTION: The 3D viewer was disabled! (-no3d argument was passed via command line)");

    //setup a timer to update the status bar message every 2 seconds.
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(onUpdateStatusBar()));
    timer->start(2000); //time specified in ms
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::displayApplicationInfo()
{
    ui->lblProjName->setText( QString("Project: ").append( Application::instance()->getOpenProjectName() ) );

    if ( Application::instance()->hasOpenProject() )
        ui->treeProject->setModel( Application::instance()->getProject() );
    else
        ui->treeProject->setModel( nullptr );
    ui->treeProject->header()->hide();
}

void MainWindow::log_message(const QString message, const QString style)
{
    if(style.compare("information") == 0)
        ui->txtedMessages->setTextColor( Qt::darkBlue );
    else if(style.compare("warning") == 0)
        ui->txtedMessages->setTextColor( Qt::darkYellow );
    else
        ui->txtedMessages->setTextColor( Qt::red );
    ui->txtedMessages->append( message );
    //ui->txtedMessages->append( "\n" );
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *e)
{
    foreach (const QUrl &url, e->mimeData()->urls()) {
        QString fileName = url.toLocalFile();
        doAddDataFile( fileName );
    }
}

void MainWindow::closeEvent(QCloseEvent *){
    //cleanup and state saving code here.
    Application::instance()->setMainWindowSplitterSetting( ui->splitter->saveState() );
    Application::instance()->setContentsMessageSplitterSetting( ui->splitter_2->saveState() );
    Application::instance()->setLastlyOpenedProjectSetting();
    this->saveProjectTreeUIState();
}

void MainWindow::openProject(const QString path)
{
    Application::instance()->openProject( path );
    this->setCurrentMRU( path );
    displayApplicationInfo();
    if( Application::instance()->hasOpenProject() )
        ui->menuEstimation->setEnabled( true );
}

void MainWindow::setCurrentMRU(const QString path)
{
    QSettings settings;
    //retrieve project paths from registry
    QStringList projects = settings.value("recentProjectList").toStringList();
    //remove entry corresponding to the given project path
    projects.removeAll(path);
    //places the given project at top
    projects.prepend(path);
    //removes any old project entries so the maximum number
    //of items is not exceeded.
    while (projects.size() > MaxRecentProjects)
        projects.removeLast();
    //saves project list to registry
    settings.setValue("recentProjectList", projects);
    //update MRU menus according to the saved list
    this->updateRecentProjectActions();
}

void MainWindow::updateRecentProjectActions()
{
    QSettings settings;
    //retrieve project paths from registry
    QStringList projects = settings.value("recentProjectList").toStringList();
    //determine number of visible MRU menu items
    int numRecentFiles = qMin(projects.size(), (int)MaxRecentProjects);
    //reconfigure menu items according to MRU project list
    for (int i = 0; i < numRecentFiles; ++i) {
        QString text = tr("&%1 %2").arg(i + 1).arg(strippedName(projects[i]));
        this->_MRUactions[i]->setText(text);
        this->_MRUactions[i]->setData(projects[i]);
        this->_MRUactions[i]->setVisible(true);
    }
    //the non-used menu items remain invisible
    for (int j = numRecentFiles; j < MaxRecentProjects; ++j)
        this->_MRUactions[j]->setVisible(false);
}

void MainWindow::saveProjectTreeUIState()
{
    QStringList List;
    // prepare list
    // PS: getPersistentIndexList() function is a simple `return this->persistentIndexList()` from TreeModel model class
    if( Application::instance()->hasOpenProject() ){
        foreach (QModelIndex index, Application::instance()->getProject()->getPersistentIndexList())
        {
            if ( ui->treeProject->isExpanded(index))
            {
                List << index.data(Qt::DisplayRole).toString(); //DisplayRole == item label (may use UserRole with an unique ID to assure uniqueness)
            }
        }
    }
    // save list
    QSettings qs;
    qs.setValue("ExpandedTreeItems", QVariant::fromValue(List));
}

void MainWindow::restoreProjectTreeUIState()
{
    QStringList List;
    // get list
    QSettings qs;
    List = qs.value("ExpandedTreeItems").toStringList();
    foreach (QString item, List)
    {
        // search `item` data in model
        QModelIndexList Items = Application::instance()->getProject()->match(Application::instance()->getProject()->index(0, 0, QModelIndex()), Qt::DisplayRole, QVariant(item));
        if (!Items.isEmpty())
        {
            // Information: with this code, expands ONLY first level in QTreeView
             ui->treeProject->setExpanded(Items.first(), true);
        }
    }
}

void MainWindow::restoreProjectTreeUIState2()
{
    QStringList List;
    // get list
    QSettings qs;
    List = qs.value("ExpandedTreeItems").toStringList();
    foreach (QString item, List)
    {
        if( Application::instance()->hasOpenProject() ){
            // search `item` data in model
            foreach (QModelIndex model_item, Application::instance()->getProject()->getIndexList( QModelIndex() )){
                if ( model_item.data( Qt::DisplayRole ).toString() == item ){
                    ui->treeProject->setExpanded(model_item, true);
                    break; //end internal loop since the item was found.
                }
            }
        }
    }
}

void MainWindow::refreshTreeStyle()
{

    if(Util::getDisplayResolutionClass() == DisplayResolution::NORMAL_DPI)
        ui->treeProject->setStyleSheet("QTreeView::branch:has-siblings:!adjoins-item { \
                                       border-image: url(:icons/vline) 0; } \
                QTreeView::branch:has-siblings:adjoins-item { \
                     border-image: url(:icons/bmore) 0; \
                 } \
                \
                 QTreeView::branch:!has-children:!has-siblings:adjoins-item { \
                     border-image: url(:icons/bend) 0; \
                 } \
                \
                 QTreeView::branch:has-children:!has-siblings:closed, \
                 QTreeView::branch:closed:has-children:has-siblings { \
                         border-image: none; \
                         image: url(:icons/bclosed); \
                 } \
                \
                 QTreeView::branch:open:has-children:!has-siblings, \
                 QTreeView::branch:open:has-children:has-siblings  { \
                         border-image: none; \
                         image: url(:icons/bopen); \
                 }");
    else
         ui->treeProject->setStyleSheet("QTreeView::branch:has-siblings:!adjoins-item { \
                                        border-image: url(:icons32/vline32) 0; } \
                 QTreeView::branch:has-siblings:adjoins-item { \
                      border-image: url(:icons32/bmore32) 0; \
                  } \
                 \
                  QTreeView::branch:!has-children:!has-siblings:adjoins-item { \
                      border-image: url(:icons32/bend32) 0; \
                  } \
                 \
                  QTreeView::branch:has-children:!has-siblings:closed, \
                  QTreeView::branch:closed:has-children:has-siblings { \
                          border-image: none; \
                          image: url(:icons32/bclosed32); \
                  } \
                 \
                  QTreeView::branch:open:has-children:!has-siblings, \
                  QTreeView::branch:open:has-children:has-siblings  { \
                          border-image: none; \
                          image: url(:icons32/bopen32); \
                  }");

}


void MainWindow::onProjectHeaderContextMenu(const QPoint &mouse_location)
{
    if( ! Application::instance()->hasOpenProject() )
        return;
    _projectHeaderContextMenu->clear(); //remove all context menu actions
    _projectHeaderContextMenu->addAction("See project path", this, SLOT(onSeeProjectPath()));
    _projectHeaderContextMenu->addAction("Open project directory...", this, SLOT(onOpenProjectPath()));
    _projectHeaderContextMenu->addAction("Clear temporary files", this, SLOT(onCleanTmpFiles()));
    _projectHeaderContextMenu->addAction("Free loaded data (frees up RAM)", this, SLOT(onFreeLoadedData()));
    _projectHeaderContextMenu->exec(ui->lblProjName->mapToGlobal(mouse_location));
}

void MainWindow::onProjectContextMenu(const QPoint &mouse_location)
{
    QModelIndex index = ui->treeProject->indexAt(mouse_location); //get index to project component under mouse
    Project* project = Application::instance()->getProject(); //get pointer to open project
    _projectContextMenu->clear(); //remove all context menu actions

    //get all selected items, this may include other items different from the one under the mouse pointer.
    QModelIndexList selected_indexes = ui->treeProject->selectionModel()->selectedIndexes();

    //if there is just one selected item.
    if( selected_indexes.size() == 1 ){
        //build context menu for the Data Files group
        if ( index.isValid() && index.internalPointer() == project->getDataFilesGroup()) {
            _projectContextMenu->addAction("Add data file...", this, SLOT(onAddDataFile()));
        }
        //build context menu for the Variograms group
        if ( index.isValid() && index.internalPointer() == project->getVariogramsGroup()) {
            _projectContextMenu->addAction("Create variogram model...", this, SLOT(onCreateVariogramModel()));
        }
        //build context menu for the Resources group
        if ( index.isValid() && index.internalPointer() == project->getResourcesGroup()) {
            _projectContextMenu->addAction("Create threshold c.d.f. ...", this, SLOT(onCreateThresholdCDF()));
            _projectContextMenu->addAction("Create categories definition ...", this, SLOT(onCreateCategoryDefinition()));
        }
        //build context menu for a file
        if ( index.isValid() && (static_cast<ProjectComponent*>( index.internalPointer() ))->isFile() ) {
            _right_clicked_file = static_cast<File*>( index.internalPointer() );
            _projectContextMenu->addAction("Remove from project", this, SLOT(onRemoveFile()));
            _projectContextMenu->addAction("Remove and delete", this, SLOT(onRemoveAndDeleteFile()));
            if( _right_clicked_file->isEditable() )
                _projectContextMenu->addAction("Edit", this, SLOT(onEdit()));
            if ( _right_clicked_file->canHaveMetaData() ){
                _projectContextMenu->addAction("See metadata", this, SLOT(onSeeMetadata()));
                //TODO: consider creating a method hasNDV() in File class.
                if( _right_clicked_file->getFileType() != "EXPVARIOGRAM" &&
                    _right_clicked_file->getFileType() != "UNIDIST" &&
                    _right_clicked_file->getFileType() != "BIDIST")
                    _projectContextMenu->addAction("Set no-data value", this, SLOT(onSetNDV()));
            }
            if( _right_clicked_file->getFileType() == "PLOT" ){
                _projectContextMenu->addAction("Display", this, SLOT(onDisplayPlot()));
            }
            if( _right_clicked_file->getFileType() == "EXPVARIOGRAM" ){
                _projectContextMenu->addAction("Plot", this, SLOT(onDisplayExperimentalVariogram()));
                _projectContextMenu->addAction("Fit variogram model...", this, SLOT(onFitVModelToExperimentalVariogram()));
            }
            if( _right_clicked_file->getFileType() == "VMODEL" ){
                _projectContextMenu->addAction("Review", this, SLOT(onDisplayVariogramModel()));
            }
            if( _right_clicked_file->getFileType() == "POINTSET" ){
                _projectContextMenu->addAction("Create estimation/simulation grid...", this, SLOT(onCreateGrid()));
                _projectContextMenu->addAction("Look for duplicate/close samples", this, SLOT(onLookForDuplicates()));
            }
            if( _right_clicked_file->getFileType() == "CARTESIANGRID" ){
                _projectContextMenu->addAction("Convert to point set", this, SLOT(onAddCoord()));
                _projectContextMenu->addAction("Resample", this, SLOT(onResampleGrid()));
            }
            if( _right_clicked_file->getFileType() == "CATEGORYDEFINITION" ){
                _projectContextMenu->addAction("Create category p.d.f. ...", this, SLOT(onCreateCategoryPDF()));
            }
            _projectContextMenu->addAction("Open with external program", this, SLOT(onEditWithExternalProgram()));
        }
        //build context menu for an attribute
        if ( index.isValid() && (static_cast<ProjectComponent*>( index.internalPointer() ))->isAttribute() ) {
            _right_clicked_attribute = static_cast<Attribute*>( index.internalPointer() );
            File* parent_file = _right_clicked_attribute->getContainingFile();
            if( parent_file->getFileType() == "POINTSET" ||
                parent_file->getFileType() == "CARTESIANGRID"  )
                _projectContextMenu->addAction("Histogram", this, SLOT(onHistogram()));
            if( parent_file->getFileType().compare("POINTSET") == 0 ){
                _projectContextMenu->addAction("Map (locmap)", this, SLOT(onLocMap()));
                _projectContextMenu->addAction("Decluster...", this, SLOT(onDecluster()));
            }
            if( parent_file->getFileType().compare("CARTESIANGRID") == 0 ){
                _projectContextMenu->addAction("Map (pixelplt)", this, SLOT(onPixelPlt()));
                makeMenuMapAs();
                _projectContextMenu->addMenu( m_subMenuMapAs );
            }
            if( parent_file->getFileType() == "POINTSET" ||
                parent_file->getFileType() == "CARTESIANGRID"  ){
                _projectContextMenu->addAction("Probability plot", this, SLOT(onProbPlt()));
                _projectContextMenu->addAction("Variogram analysis...", this, SLOT(onVariogramAnalysis()));
                _projectContextMenu->addAction("Normal score...", this, SLOT(onNScore()));
                _projectContextMenu->addAction("Model a distribution...", this, SLOT(onDistrModel()));
            }
            if( parent_file->getFileType().compare("POINTSET") == 0 ){
                makeMenuClassifyInto();
                _projectContextMenu->addMenu( m_subMenuClassifyInto );
                makeMenuClassifyWith();
                _projectContextMenu->addMenu( m_subMenuClassifyWith );
                _projectContextMenu->addAction("Soft indicator calibration...", this, SLOT(onSoftIndicatorCalib()) );
            }
            if( parent_file->getFileType() == "CARTESIANGRID"  ){
                _projectContextMenu->addAction("FFT", this, SLOT(onFFT()));
                _projectContextMenu->addAction("NDV estimation", this, SLOT(onNDVEstimation()));
                CartesianGrid* cg = (CartesianGrid*)parent_file;
                if( cg->getNReal() > 1){ //if parent file is Cartesian grid and has more than one realization
                    _right_clicked_attribute2 = nullptr; //onHistpltsim() is also used with two attributes selected
                    _projectContextMenu->addAction("Realizations histograms", this, SLOT(onHistpltsim()));
                }
            }
        }
    //two items were selected.  The context menu depends on the combination of items.
    } else if ( selected_indexes.size() == 2 ) {
        QModelIndex index1 = selected_indexes.first();
        QModelIndex index2 = selected_indexes.last();
        //if both objects are variables and have the same parent file,
        if( index1.isValid() && index2.isValid() &&
            (static_cast<ProjectComponent*>( index1.internalPointer() ))->isAttribute() &&
            (static_cast<ProjectComponent*>( index2.internalPointer() ))->isAttribute() &&
            (static_cast<ProjectComponent*>( index1.internalPointer() ))->getParent() ==
            (static_cast<ProjectComponent*>( index2.internalPointer() ))->getParent()
          ){
            QString menu_caption_xplot = "Cross plot ";
            QString menu_caption_bidist = "Model bidistribution ";
            QString menu_caption_xvariography = "Cross variography ";
            QString menu_caption_vars = (static_cast<ProjectComponent*>( index1.internalPointer() ))->getName();
            menu_caption_vars.append(" X ");
            menu_caption_vars.append((static_cast<ProjectComponent*>( index2.internalPointer() ))->getName());
            _projectContextMenu->addAction(menu_caption_xplot.append(menu_caption_vars), this, SLOT(onXPlot()));
            _projectContextMenu->addAction(menu_caption_bidist.append(menu_caption_vars), this, SLOT(onBidistrModel()));
            _projectContextMenu->addAction(menu_caption_xvariography.append(menu_caption_vars), this, SLOT(onVariogramAnalysis()));
            //if their parent file is a Cartesin grid
            _right_clicked_attribute = static_cast<Attribute*>( index1.internalPointer() );
            _right_clicked_attribute2 = static_cast<Attribute*>( index2.internalPointer() );
            if( _right_clicked_attribute->getContainingFile()->getFileType() == "CARTESIANGRID" ){
                QString menu_caption_rfft = "rev. FFT: ";
                menu_caption_rfft += "mag. = " + _right_clicked_attribute->getName();
                menu_caption_rfft += "; phase = " + _right_clicked_attribute2->getName();
                _projectContextMenu->addAction(menu_caption_rfft, this, SLOT(onRFFT()));
            }
        }
        //if both objects are variables and have different parent files,
        if( index1.isValid() && index2.isValid() &&
            (static_cast<ProjectComponent*>( index1.internalPointer() ))->isAttribute() &&
            (static_cast<ProjectComponent*>( index2.internalPointer() ))->isAttribute() &&
            (static_cast<Attribute*>( index1.internalPointer() ))->getContainingFile() !=
            (static_cast<Attribute*>( index2.internalPointer() ))->getContainingFile()
          ){
            _right_clicked_attribute = static_cast<Attribute*>( index1.internalPointer() );
            _right_clicked_attribute2 = static_cast<Attribute*>( index2.internalPointer() );
            _projectContextMenu->addAction("Q-Q/P-P plot", this, SLOT(onQpplt()));
            //if one parent is a point set and the other is a Cartesian grid
            if( (_right_clicked_attribute ->getContainingFile()->getFileType()=="POINTSET" &&
                 _right_clicked_attribute2->getContainingFile()->getFileType()=="CARTESIANGRID") ||
                (_right_clicked_attribute2->getContainingFile()->getFileType()=="POINTSET" &&
                 _right_clicked_attribute ->getContainingFile()->getFileType()=="CARTESIANGRID")){
                CartesianGrid* cg;
                if( _right_clicked_attribute2->getContainingFile()->getFileType()=="CARTESIANGRID" )
                    cg = (CartesianGrid*)_right_clicked_attribute2->getContainingFile();
                else
                    cg = (CartesianGrid*)_right_clicked_attribute->getContainingFile();
                if( cg->getNReal() > 1)
                    _projectContextMenu->addAction("Realizations histograms", this, SLOT(onHistpltsim()));
            }
        }
        //if one object is a cartesian grid and the other object is a point set
        // then Get Points can be called, which
        //consists of creating a new point set that is a copy of the original point set but with
        //the collocated values of the selected variable taken from the grid.
        if( index1.isValid() && index2.isValid() ){

            File* file1 = nullptr;
            File* file2 = nullptr;
            if( (static_cast<ProjectComponent*>( index1.internalPointer() ))->isFile() ){
                file1 = static_cast<File*>( index1.internalPointer() );
            }
            if( (static_cast<ProjectComponent*>( index2.internalPointer() ))->isFile() ){
                file2 = static_cast<File*>( index2.internalPointer() );
            }


            //determine the destination PointSet file and origin CartesianGrid
            PointSet* point_set = nullptr;
            CartesianGrid* cartesian_grid = nullptr;
            if( file1 ){
                if( file1->getFileType() == "POINTSET" )
                    point_set = static_cast<PointSet*>( file1 );
                else if( file1->getFileType() == "CARTESIANGRID" )
                    cartesian_grid = static_cast<CartesianGrid*>( file1 );
            }
            if( file2 ){
                if( file2->getFileType() == "POINTSET" )
                    point_set = static_cast<PointSet*>( file2 );
                else if( file2->getFileType() == "CARTESIANGRID" )
                    cartesian_grid = static_cast<CartesianGrid*>( file2 );
            }

            //if user selected a point set file and a grid
            if( point_set && cartesian_grid ){
                _right_clicked_cartesian_grid = cartesian_grid;
                _right_clicked_point_set = point_set;
                QString menu_caption = "Transfer collocated values from ";
                menu_caption.append(cartesian_grid->getName());
                menu_caption.append(" to ");
                menu_caption.append( point_set->getName());
                _projectContextMenu->addAction(menu_caption, this, SLOT(onGetPoints()));
            }
        }
    //three items were selected.  The context menu depends on the combination of items.
    } else if ( selected_indexes.size() == 3 ) {
        QModelIndex index1 = selected_indexes.first();
        QModelIndex index2 = selected_indexes.at(1);
        QModelIndex index3 = selected_indexes.last();
        //if all objects are variables and have the same parent file, then they can be cross-plotted
        if( index1.isValid() && index2.isValid() && index3.isValid() &&
            (static_cast<ProjectComponent*>( index1.internalPointer() ))->isAttribute() &&
            (static_cast<ProjectComponent*>( index2.internalPointer() ))->isAttribute() &&
            (static_cast<ProjectComponent*>( index3.internalPointer() ))->isAttribute() &&
            (static_cast<ProjectComponent*>( index1.internalPointer() ))->getParent() ==
            (static_cast<ProjectComponent*>( index2.internalPointer() ))->getParent()   &&
            (static_cast<ProjectComponent*>( index2.internalPointer() ))->getParent() ==
            (static_cast<ProjectComponent*>( index3.internalPointer() ))->getParent()
          ){
            QString menu_caption = "Cross plot ";
            menu_caption.append((static_cast<ProjectComponent*>( index1.internalPointer() ))->getName());
            menu_caption.append(" X ");
            menu_caption.append((static_cast<ProjectComponent*>( index2.internalPointer() ))->getName());
            menu_caption.append(" X ");
            menu_caption.append((static_cast<ProjectComponent*>( index3.internalPointer() ))->getName());
            _projectContextMenu->addAction(menu_caption, this, SLOT(onXPlot()));
        }
    }

    //variable combinations of selected items
    {
        //determine whether all selected items are Attributes
        bool areAllItemsAttributes = true;
        QModelIndexList::iterator it = selected_indexes.begin();
        for(;  it != selected_indexes.end(); ++it){
            QModelIndex index = *it;
            if( !index.isValid() || !(static_cast<ProjectComponent*>( index.internalPointer() ))->isAttribute() ){
                areAllItemsAttributes = false;
            }
        }
        //if all selected items are attributes (two or more)
        if( areAllItemsAttributes && selected_indexes.size() > 1 ){
            _projectContextMenu->addAction("Multiple variograms", this, SLOT(onMultiVariogram()));
        }
    }

    //show the context menu under the mouse cursor.
    if( _projectContextMenu->actions().size() > 0 )
        _projectContextMenu->exec(ui->treeProject->mapToGlobal(mouse_location));
}


void MainWindow::onAddDataFile()
{
    QString file = QFileDialog::getOpenFileName(this, "select data file", Util::getLastBrowsedDirectory());
    doAddDataFile( file );
}

void MainWindow::onRemoveFile()
{
    int ret = QMessageBox::warning(this, "Confirm removal operation",
                                    QString("Remove ").append(_right_clicked_file->getName()).append(" from project (file will remain on file system)?"),
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No);
    if( ret == QMessageBox::Yes ){
        Application::instance()->getProject()->removeFile( _right_clicked_file, false); //remove item from project data structure
        this->refreshTreeStyle(); //update tree drawing
        this->restoreProjectTreeUIState2(); //preserve tree state (collapsed/expanded items)
        Application::instance()->getProject()->save(); //persist project changes
    }
}

void MainWindow::onRemoveAndDeleteFile()
{
    int ret = QMessageBox::warning(this, "Confirm removal operation",
                                    QString("Delete ").append(_right_clicked_file->getName()).append(" (and remove from project)?"),
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No);
    if( ret == QMessageBox::Yes ){
        Application::instance()->getProject()->removeFile( _right_clicked_file, true); //remove item from project data structure
        this->refreshTreeStyle(); //update tree drawing
        this->restoreProjectTreeUIState2(); //preserve tree state (collapsed/expanded items)
        Application::instance()->getProject()->save(); //persist project changes
    }
}

void MainWindow::onSeeMetadata()
{
    FileContentsDialog fcd( this, _right_clicked_file->getMetaDataFilePath(), QString(_right_clicked_file->getFileName()).append(" metadata."));
    fcd.exec();
}

void MainWindow::onHistogram()
{
    Util::viewHistogram( _right_clicked_attribute, this );
}

void MainWindow::onLocMap()
{
    Util::viewPointSet( _right_clicked_attribute, this );
}

void MainWindow::onXPlot()
{
    //get the selected attributes
    QList<Attribute*> selected_indexes = getSelectedAttributes();
    Attribute* var1 = selected_indexes.at( 0 );
    Attribute* var2 = selected_indexes.at( 1 );
    Attribute* var3 = nullptr;
    if( selected_indexes.size() == 3)
        var3 = selected_indexes.at( 2 );

    Util::viewXPlot( var1, var2, this, var3 );
}

void MainWindow::onPixelPlt()
{
    CategoryDefinition *cd = nullptr;
    //if the attribute is categorical...
    if(  _right_clicked_attribute->isCategorical() ){
        //the parent file is surely a CartesianGrid.
        CartesianGrid *cg = (CartesianGrid*)_right_clicked_attribute->getContainingFile();
        //... get the associated category definition
        cd = cg->getCategoryDefinition( _right_clicked_attribute );
    }
    Util::viewGrid( _right_clicked_attribute, this, false, cd );
}

void MainWindow::onProbPlt()
{
    //get input data file
    DataFile* input_data_file = (DataFile*)_right_clicked_attribute->getContainingFile();

    //load file data.
    input_data_file->loadData();

    //get the variable index in parent data file
    uint var_index = input_data_file->getFieldGEOEASIndex( _right_clicked_attribute->getName() );

    //make plot/window title
    QString title = _right_clicked_attribute->getContainingFile()->getName();
    title.append("/");
    title.append(_right_clicked_attribute->getName());
    title.append(" probability plot");

    //Construct an object composition based on the parameter file template for the probplt program.
    GSLibParameterFile gpf( "probplt" );

    //Set default values so we need to change less parameters and let
    //the user change the others as one may see fit.
    gpf.setDefaultValues();

    //get the maximum and minimun of selected variable with some tolerance,
    //excluding no-data values
    double data_min = input_data_file->min( var_index - 1 );
    double data_max = input_data_file->max( var_index - 1 );
    data_min -= fabs( data_min / 100.0 );
    data_max += fabs( data_max / 100.0 );

    //---------------------set the parameters-----------------------------

    //set the input data
    GSLibParInputData* par0;
    par0 = gpf.getParameter<GSLibParInputData*>(0);
    par0->_file_with_data._path = input_data_file->getPath();
    par0->_trimming_limits._max = data_max;
    par0->_trimming_limits._min = data_min;
    par0->_var_wgt_pairs.first()->_var_index = var_index;

    //set the output PS file
    gpf.getParameter<GSLibParFile*>(1)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("ps");

    //set the scale
    GSLibParMultiValuedFixed *par4 = gpf.getParameter<GSLibParMultiValuedFixed*>(4);
    par4->getParameter<GSLibParDouble*>(0)->_value = data_min;
    par4->getParameter<GSLibParDouble*>(1)->_value = data_max;
    par4->getParameter<GSLibParDouble*>(2)->_value = fabs( data_max - data_min ) / 10.0;

    //set the plot title
    gpf.getParameter<GSLibParString*>(5)->_value = title;
    //----------------------------------------------------------------------------------

    //Generate the parameter file
    QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
    gpf.save( par_file_path );

    //run probplt program
    Application::instance()->logInfo("Starting probplt program...");
    GSLib::instance()->runProgram( "probplt", par_file_path );

    //display the plot output
    DisplayPlotDialog *dpd = new DisplayPlotDialog(gpf.getParameter<GSLibParFile*>(1)->_path, title, gpf, this);
    dpd->show(); //show() makes dialog modalless
}

void MainWindow::onQpplt()
{
    //get input data files
    DataFile* input_data_fileX = (DataFile*)_right_clicked_attribute->getContainingFile();
    DataFile* input_data_fileY = (DataFile*)_right_clicked_attribute2->getContainingFile();

    //load file2 data.
    input_data_fileX->loadData();
    input_data_fileY->loadData();

    //get the variable indexes in parent data files
    uint var_indexX = input_data_fileX->getFieldGEOEASIndex( _right_clicked_attribute->getName() );
    uint var_indexY = input_data_fileY->getFieldGEOEASIndex( _right_clicked_attribute2->getName() );

    //make plot/window title
    QString title = "Q-Q/P-P plot ";
    title.append( input_data_fileX->getName() );
    title.append(" X ");
    title.append( input_data_fileY->getName() );
    title.append("(" + _right_clicked_attribute->getName() + ")"); //assumes the variable is the same in both files

    //Construct an object composition based on the parameter file template for the qpplt program.
    GSLibParameterFile gpf( "qpplt" );

    //Set default values so we need to change less parameters and let
    //the user change the others as one may see fit.
    gpf.setDefaultValues();

    //get the maximum and minimun of selected variables with some tolerance,
    //excluding no-data values
    double data_minX = input_data_fileX->min( var_indexX - 1 );
    double data_maxX = input_data_fileX->max( var_indexX - 1 );
    data_minX -= fabs( data_minX / 100.0 );
    data_maxX += fabs( data_maxX / 100.0 );
    double data_minY = input_data_fileY->min( var_indexY - 1 );
    double data_maxY = input_data_fileY->max( var_indexY - 1 );
    data_minY -= fabs( data_minY / 100.0 );
    data_maxY += fabs( data_maxY / 100.0 );

    //---------------------set the parameters-----------------------------

    //set the first input file
    gpf.getParameter<GSLibParFile*>(0)->_path = input_data_fileX->getPath();

    //set the GEO-EAS index of first variable
    GSLibParMultiValuedFixed *par1 = gpf.getParameter<GSLibParMultiValuedFixed*>(1);
    par1->getParameter<GSLibParUInt*>(0)->_value = var_indexX;
    par1->getParameter<GSLibParUInt*>(1)->_value = 0;

    //set the second input file
    gpf.getParameter<GSLibParFile*>(2)->_path = input_data_fileY->getPath();

    //set the GEO-EAS index of second variable
    GSLibParMultiValuedFixed *par3 = gpf.getParameter<GSLibParMultiValuedFixed*>(3);
    par3->getParameter<GSLibParUInt*>(0)->_value = var_indexY;
    par3->getParameter<GSLibParUInt*>(1)->_value = 0;

    //set the trimming limits
    GSLibParMultiValuedFixed *par4 = gpf.getParameter<GSLibParMultiValuedFixed*>(4);
    par4->getParameter<GSLibParDouble*>(0)->_value = ( data_minX < data_minY ? data_minX : data_minY );
    par4->getParameter<GSLibParDouble*>(1)->_value = ( data_maxX > data_maxY ? data_maxX : data_maxY );

    //set the output PostScript file
    gpf.getParameter<GSLibParFile*>(5)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("ps");

    //set the X axis range
    GSLibParMultiValuedFixed *par8 = gpf.getParameter<GSLibParMultiValuedFixed*>(8);
    par8->getParameter<GSLibParDouble*>(0)->_value = data_minX;
    par8->getParameter<GSLibParDouble*>(1)->_value = data_maxX;

    //set the Y axis range
    GSLibParMultiValuedFixed *par9 = gpf.getParameter<GSLibParMultiValuedFixed*>(9);
    par9->getParameter<GSLibParDouble*>(0)->_value = data_minY;
    par9->getParameter<GSLibParDouble*>(1)->_value = data_maxY;

    //set the plot title
    gpf.getParameter<GSLibParString*>(11)->_value = title;

    //----------------------------------------------------------------------------------

    //Generate the parameter file
    QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
    gpf.save( par_file_path );

    //run qpplt program
    Application::instance()->logInfo("Starting qpplt program...");
    GSLib::instance()->runProgram( "qpplt", par_file_path );

    //display the plot output
    DisplayPlotDialog *dpd = new DisplayPlotDialog(gpf.getParameter<GSLibParFile*>(5)->_path, title, gpf, this);
    dpd->show(); //show() makes dialog modalless
}


void MainWindow::onSeeProjectPath()
{
    QMessageBox::information(this, "Project path", Application::instance()->getProject()->getPath());
}

void MainWindow::onOpenProjectPath()
{
    QFileDialog::getOpenFileName(this, "Project directory", Application::instance()->getProject()->getPath());
}

void MainWindow::onVariogramAnalysis()
{
    //get the selected attributes
    QList<Attribute*> selected_indexes = getSelectedAttributes();
    Attribute* var1 = selected_indexes.at( 0 );
    Attribute* var2 = var1; //auto-variography by default
    if( selected_indexes.size() > 1 )
        var2 = selected_indexes.at( 1 ); //cross variography

    VariogramAnalysisDialog* vad = new VariogramAnalysisDialog( var1, var2, this );
    vad->setAttribute( Qt::WA_DeleteOnClose );
    vad->show();
}

void MainWindow::onDecluster()
{
    DeclusteringDialog* dd = new DeclusteringDialog( _right_clicked_attribute, this );
    dd->setAttribute( Qt::WA_DeleteOnClose );
    dd->show();
}

void MainWindow::onSetNDV()
{
    DataFile* data_file = (DataFile*)_right_clicked_file;

    bool ok;
    QString new_ndv = QInputDialog::getText(this, "Set no-data value",
                                             "Current no-data value (blank == not set):", QLineEdit::Normal,
                                             data_file->getNoDataValue(), &ok);
    if (ok && !new_ndv.isEmpty()){
        data_file->setNoDataValue( new_ndv );
    }
}

void MainWindow::onGetPoints()
{
    PointSet* point_set = _right_clicked_point_set;
    CartesianGrid* cg_grid = _right_clicked_cartesian_grid;

    //load grid data
    cg_grid->loadData();

    //create the GSLib parameter object
    GSLibParameterFile gpf( "getpoints" );

    //the grid file
    gpf.getParameter<GSLibParFile*>(0)->_path = cg_grid->getPath();

    //the grid geometry
    GSLibParMultiValuedFixed* par1;
    par1 = gpf.getParameter<GSLibParMultiValuedFixed*>(1);
    par1->getParameter<GSLibParUInt*>(0)->_value = cg_grid->getNX();
    par1->getParameter<GSLibParDouble*>(1)->_value = cg_grid->getX0();
    par1->getParameter<GSLibParDouble*>(2)->_value = cg_grid->getDX();
    GSLibParMultiValuedFixed* par2;
    par2 = gpf.getParameter<GSLibParMultiValuedFixed*>(2);
    par2->getParameter<GSLibParUInt*>(0)->_value = cg_grid->getNY();
    par2->getParameter<GSLibParDouble*>(1)->_value = cg_grid->getY0();
    par2->getParameter<GSLibParDouble*>(2)->_value = cg_grid->getDY();

    //the point set file
    gpf.getParameter<GSLibParFile*>(3)->_path = point_set->getPath();

    //columns for X and Y
    GSLibParMultiValuedFixed* par4;
    par4 = gpf.getParameter<GSLibParMultiValuedFixed*>(4);
    par4->getParameter<GSLibParUInt*>(0)->_value = point_set->getXindex();
    par4->getParameter<GSLibParUInt*>(1)->_value = point_set->getYindex();

    //the output file
    gpf.getParameter<GSLibParFile*>(5)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("out");

    //Generate the parameter file
    QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
    gpf.save( par_file_path );

    //run getpoints program
    Application::instance()->logInfo("Starting getpoints program...");
    GSLib::instance()->runProgram( "getpoints", par_file_path );
    Application::instance()->logInfo("getpoints completed.");

    //call rename to trim trailing spaces that getpoints leaves in varable names
    Util::renameGEOEASvariable( gpf.getParameter<GSLibParFile*>(5)->_path, "aaaaaaaaaaa", "aaaaaaaaaaa");

    //update the point set file with the new file with the collocated variables transfered
    //from the cartesian grid.
    point_set->replacePhysicalFile( gpf.getParameter<GSLibParFile*>(5)->_path );
}

void MainWindow::onNScore()
{
    //open n-score dialog passing the selected properties
    NScoreDialog* nsd = new NScoreDialog( _right_clicked_attribute, this );
    nsd->setAttribute( Qt::WA_DeleteOnClose );
    nsd->show();
}

void MainWindow::onDisplayPlot()
{
    //display the plot output
    DisplayPlotDialog *dpd = new DisplayPlotDialog( _right_clicked_file->getPath() , "Display saved plot.", GSLibParameterFile(), this);
    dpd->show(); //show() makes dialog modalless
}

void MainWindow::onDisplayExperimentalVariogram()
{
    //get pointer to the experimental variogram file
    ExperimentalVariogram* ev = (ExperimentalVariogram*)_right_clicked_file;

    //get the path to the vargplt parameter file associated with the experimental variogram object
    QString vargplt_par_path = ev->getPathToVargpltPar();

    //checks whether the parameter file exists
    if( vargplt_par_path.trimmed().isEmpty() ){
        QMessageBox::warning(this, "Error", "The experimental variogram does not have an associated vargplt parameter file.");
        return;
    } else {
        QFile f( vargplt_par_path );
        if( ! f.exists() ){
            QMessageBox::warning(this, "Error", "The associated vargplt parameter file does not exist.");
            return;
        }
    }

    //run vargplt program
    Application::instance()->logInfo("Starting vargplt program...");
    GSLib::instance()->runProgram( "vargplt", vargplt_par_path );

    //construct a parameter file object for the vargplt program.
    GSLibParameterFile gpf( "vargplt" );

    //fills the values from the saved vargplt file.
    gpf.setValuesFromParFile( vargplt_par_path );

    //display the plot output
    DisplayPlotDialog *dpd = new DisplayPlotDialog(gpf.getParameter<GSLibParFile*>(0)->_path,
                                                   gpf.getParameter<GSLibParString*>(5)->_value,
                                                   gpf,
                                                   this);
    dpd->show();
}

void MainWindow::onFitVModelToExperimentalVariogram()
{
    //open the variogram analysis dialog in experimental variogram fitting.
    VariogramAnalysisDialog* vad = new VariogramAnalysisDialog( (ExperimentalVariogram*)_right_clicked_file,
                                                                this );
    vad->setAttribute( Qt::WA_DeleteOnClose );
    vad->show();
}

void MainWindow::onDisplayVariogramModel()
{
    //get pointer to the variogram model object right-clicked by the user
    VariogramModel* vm = (VariogramModel*)_right_clicked_file;
    this->createOrReviewVariogramModel( vm );
}

void MainWindow::onCreateVariogramModel()
{
    this->createOrReviewVariogramModel( nullptr );
}

void MainWindow::onCreateGrid()
{
    CreateGridDialog* cgd = new CreateGridDialog( (PointSet*)_right_clicked_file, this );
    cgd->setAttribute( Qt::WA_DeleteOnClose );
    cgd->show();
}

void MainWindow::onCleanTmpFiles()
{
    QString dir = Application::instance()->getProject()->getTmpPath();
    quint64 dir_size = Util::getDirectorySize( dir );
    uint size_MB = dir_size / 1024 / 1024;
    int reply = QMessageBox::question(this, "Confirm operation", "Remove all project's temporary files (" + dir + ": " + QString::number( size_MB ) + " MB)?",
                                      QMessageBox::No | QMessageBox::Yes, QMessageBox::No);
    if( reply == QMessageBox::Yes){
        Util::clearDirectory( dir );
    }
}

void MainWindow::onAddCoord()
{
    //Get the Cartesian grid object.
    CartesianGrid* cg = (CartesianGrid*)_right_clicked_file;

    //ask the user for the realization number to select
    bool ok = false;
    uint nreal = cg->getNReal();
    int realNumber = QInputDialog::getInt(this, "Set realization number",
                      "realization number (count=" + QString::number(nreal) + "):", 1, 1, nreal, 1,
                       &ok);
    if(!ok) return;

    //ask the user for the name
    QString proposed_name = cg->getName().append(".xyz");
    QString new_name = QInputDialog::getText(this, "Name the new point set file",
                                             "New point set file name:", QLineEdit::Normal,
                                             proposed_name, &ok);
    if(!ok) return;

    //construct a parameter file for the addcoord program
    GSLibParameterFile gpf( "addcoord" );

    //input file
    GSLibParFile* par0 = gpf.getParameter<GSLibParFile*>( 0 );
    par0->_path = cg->getPath();

    //output file (temporary)
    GSLibParFile* par1 = gpf.getParameter<GSLibParFile*>( 1 );
    par1->_path = Application::instance()->getProject()->generateUniqueTmpFilePath( "xyz" );

    //realization number
    GSLibParUInt* par2 = gpf.getParameter<GSLibParUInt*>( 2 );
    par2->_value = realNumber;

    //grid parameters
    GSLibParGrid* par3 = gpf.getParameter<GSLibParGrid*>( 3 );
    par3->setFromCG( cg );

    //Generate the parameter file
    QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
    gpf.save( par_file_path );

    //run addcoord program
    Application::instance()->logInfo("Starting addcoord program...");
    GSLib::instance()->runProgram( "addcoord", par_file_path );
    Application::instance()->logInfo("addcoord completed.");

    //rename the output point set file
    QString newPSpath = Util::renameFile( par1->_path, new_name );

    //make the point set object from the temporary point set file created by addcoord
    PointSet *ps = new PointSet( newPSpath );

    //addcoord always adds the X,Y,Z fields as 1st, 2nd and 3rd variables in the data file
    //the ndv value is the same as the original Cartesian grid.
    ps->setInfo( 1, 2, 3, cg->getNoDataValue() );

    //adds the point set to the project
    Application::instance()->getProject()->addDataFile( ps );

    //deletes the renamed file in the temporary directory
    QFile newPSfile( newPSpath );
    newPSfile.remove();

    //refreshes the project tree
    Application::instance()->refreshProjectTree();
}

void MainWindow::onDistrModel()
{
    DistributionModelingDialog* dmd = new DistributionModelingDialog( _right_clicked_attribute, this );
    dmd->show();
}

void MainWindow::onBidistrModel()
{
    //get the selected attributes
    QList<Attribute*> selected_indexes = getSelectedAttributes();
    Attribute* var1 = selected_indexes.at( 0 );
    Attribute* var2 = selected_indexes.at( 1 );
    BidistributionModelingDialog* bdmd = new BidistributionModelingDialog( var1, var2, this );
    bdmd->show();
}

void MainWindow::onCreateThresholdCDF()
{
    ThresholdCDF* tcdf = new ThresholdCDF("");
    ValuesPairsDialog* vpd = new ValuesPairsDialog( tcdf, this );
    vpd->show();
}

void MainWindow::onEdit()
{
    if( _right_clicked_file->getFileType() == "THRESHOLDCDF" ){
        //Get the threshold c.d.f. object.
        ThresholdCDF* tcdf  = (ThresholdCDF*)_right_clicked_file;
        ValuesPairsDialog* vpd = new ValuesPairsDialog( tcdf, this );
        vpd->show();
    } else if( _right_clicked_file->getFileType() == "CATEGORYPDF" ){
        //Get the category p.d.f. object.
        CategoryPDF* cpdf  = (CategoryPDF*)_right_clicked_file;
        ValuesPairsDialog* vpd = new ValuesPairsDialog( cpdf, this );
        vpd->show();
    } else if( _right_clicked_file->getFileType() == "CATEGORYDEFINITION" ){
        //Get the category definition object.
        CategoryDefinition* cdp  = (CategoryDefinition*)_right_clicked_file;
        TriadsEditorDialog* ted = new TriadsEditorDialog( cdp, this );
        ted->show();
    } else if( _right_clicked_file->getFileType() == "UNIVARIATECATEGORYCLASSIFICATION" ){
        //Get the category definition object.
        UnivariateCategoryClassification* ucc  = (UnivariateCategoryClassification*)_right_clicked_file;
        TriadsEditorDialog* ted = new TriadsEditorDialog( ucc, this );
        ted->show();
    }
}

void MainWindow::onCreateCategoryPDF()
{
    //We can assume the file is a category definitio.
    CategoryDefinition* cd  = (CategoryDefinition*)_right_clicked_file;
    cd->loadTriplets();

    //Create an empty p.d.f.
    CategoryPDF* cpdf = new CategoryPDF(cd, "");

    //split 100% between the categories
    double parcel = 1.0 / cd->getCategoryCount();

    //init the p.d.f. with default values for each category
    for( int i = 0; i < cd->getCategoryCount(); ++i ){
        cpdf->addPair( cd->getCategoryCode( i ), parcel );
    }

    //create and show the edit dialog
    ValuesPairsDialog* vpd = new ValuesPairsDialog( cpdf, this );
    vpd->show();
}

void MainWindow::onLookForDuplicates()
{
    bool ok;
    double tolerance = QInputDialog::getDouble(this, "Duplicate tolerance",
                                             "Enter the location tolerance around the X,Y,Z coordinates:",
                                             0.1, 0.0, 10.0, 3, &ok);

    if(! ok ) return;

    double distance = QInputDialog::getDouble(this, "Duplicate separation",
                                             "Enter the distance between two points to be considered too close:",
                                             0.001, 0.0, 1000.0, 3, &ok);
    if( ok ){
        PointSet* ps = (PointSet*)_right_clicked_file;
        SpatialIndexPoints::fill( ps, tolerance );
        uint totFileDataLines = ps->getDataLineCount();
        uint headerLineCount = Util::getHeaderLineCount( ps->getPath() );
        Application::instance()->logInfo( "=======BEGIN OF REPORT============" );
        QStringList messages;
        for( uint iFileDataLine = 0; iFileDataLine < totFileDataLines; ++iFileDataLine){
            QList<uint> nearSamples = SpatialIndexPoints::getNearestWithin( iFileDataLine, 5, distance);
            QList<uint>::iterator it = nearSamples.begin();
            for(; it != nearSamples.end(); ++it){
                uint lineNumber1 = iFileDataLine + 1 + headerLineCount;
                uint lineNumber2 = *it + 1 + headerLineCount;
                //do not report symmetrical occurences.
                if( lineNumber1 < lineNumber2 )
                    messages.append( "Sample at line " + QString::number( lineNumber1 ) +
                                          " is too close to sample at line " + QString::number( lineNumber2 ) + "." );
            }
        }
        for( int i = 0; i < messages.count(); ++i){
            Application::instance()->logInfo( messages[i] );
        }
        Application::instance()->logInfo( "=======END OF REPORT============" );
    }
}

void MainWindow::onEditWithExternalProgram()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile( _right_clicked_file->getPath() ));
}

void MainWindow::onClearMessages()
{
    ui->txtedMessages->setText("");
}

void MainWindow::onClassifyWith()
{
    //assuming sender() returns a QAction* if execution passes through here.
    QAction *act = (QAction*)sender();

    //assuming the text in the menu item is the name of a univariate category classification file.
    QString uccFileName = act->text();

    //try to get the corresponding project component.
    ProjectComponent* pc = Application::instance()->getProject()->
            getResourcesGroup()->getChildByName( uccFileName );
    if( ! pc ){
        Application::instance()->logError("MainWindow::onClassifyWith(): File " + uccFileName +
                                          " not found in Resource Files group.");
        return;
    }

    //Assuming the project component is an UnivariateCategoryClassification
    m_ucc = (UnivariateCategoryClassification*)pc;

    //Open the dialog to edit the classification intervals and category.
    TriadsEditorDialog *ted = new TriadsEditorDialog( m_ucc, this );
    ted->setWindowTitle( "Classify " + _right_clicked_attribute->getName() + " with " + m_ucc->getName() );
    ted->showOKbutton();
    connect( ted, SIGNAL(accepted()), this, SLOT(onPerformClassifyInto()));
    ted->show(); //show()->non-modal / execute()->modal
    //method onPerformClassifyInto() will be called upon dilog accept.
}

void MainWindow::onMapAs()
{
    //assuming sender() returns a QAction* if execution passes through here.
    QAction *act = (QAction*)sender();

    //assuming the text in the menu item is the name of a categorical definition file.
    QString categoricalDefinitionFileName = act->text();

    //try to get the corresponding project component.
    ProjectComponent* pc = Application::instance()->getProject()->
            getResourcesGroup()->getChildByName( categoricalDefinitionFileName );
    if( ! pc ){
        Application::instance()->logError("MainWindow::onMapAs(): File " + categoricalDefinitionFileName +
                                          " not found in Resource Files group.");
        return;
    }

    //Assuming the project component is a CategoryDefinition
    CategoryDefinition* cd = (CategoryDefinition*)pc;

    //plot the cartesian grid
    Util::viewGrid( _right_clicked_attribute, this, false, cd );
}

void MainWindow::onSoftIndicatorCalib()
{
    SoftIndicatorCalibrationDialog *sicd = new SoftIndicatorCalibrationDialog( _right_clicked_attribute, this );
    sicd->show();
}

void MainWindow::onFreeLoadedData()
{
    Application::instance()->getProject()->freeLoadedData();
}

void MainWindow::onFFT()
{
    //propose a name for the new grid to contain the FFT image
    QString proposed_name = _right_clicked_attribute->getName() + "_FFT.dat";

    //user enters the name for the new grid with FFT image
    QString new_cg_name = QInputDialog::getText(this, "Name the new grid",
                                             "Name for the grid with FFT image:", QLineEdit::Normal,
                                             proposed_name );

    //if the user canceled the input box
    if ( new_cg_name.isEmpty() ){
        //abort
        return;
    }

    //the parent file is surely a CartesianGrid.
    CartesianGrid *cg = (CartesianGrid*)_right_clicked_attribute->getContainingFile();

    //get the array containing the data
    std::vector< std::complex<double> > array = cg->getArray( _right_clicked_attribute->getAttributeGEOEASgivenIndex()-1 );

    //run FFT
    Util::fft3D( cg->getNX(),
                 cg->getNY(),
                 cg->getNZ(),
                 array,
                 FFTComputationMode::DIRECT,
                 FFTImageType::POLAR_FORM );

    //make a tmp file path
    QString tmp_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("dat");

    //crate a new cartesian grid pointing to the tmp path
    CartesianGrid * new_cg = new CartesianGrid( tmp_file_path );

    //set the geometry info based on the original grid
    new_cg->setInfoFromOtherCG( cg, false );

    //save the results in the project's tmp directory
    Util::createGEOEASGrid( "Magnitude", "Angle", array, tmp_file_path);

    //import the saved file to the project
    Application::instance()->getProject()->importCartesianGrid( new_cg, new_cg_name );

    Application::instance()->logInfo("FFT completed.");
}

void MainWindow::onNDVEstimation()
{
    NDVEstimationDialog* ndved = new NDVEstimationDialog( _right_clicked_attribute, this );
    ndved->show();
}

void MainWindow::onResampleGrid()
{
    //========================user input part===============================

    //Get the Cartesian grid object.
    CartesianGrid* cg = (CartesianGrid*)_right_clicked_file;

    //Get sampling rates from user
    GridResampleDialog grd( cg, this );

    int result = grd.exec(); //wait for user response

    //if the user canceled the dialog
    if( result != QDialog::Accepted )
        return;

    //propose a name for the new grid to contain the FFT image
    QString proposed_name = _right_clicked_file->getName() + "_RESAMPLED.dat";

    //user enters the name for the new grid with FFT image
    QString new_cg_name = QInputDialog::getText(this, "Name the new grid",
                                             "Name for the resampled grid:", QLineEdit::Normal,
                                             proposed_name );

    //if the user canceled the input box
    if ( new_cg_name.isEmpty() ){
        //abort
        return;
    }

    //=====================if user didn't cancel, proceed to task===================

    int iIrate = grd.getIrate();
    int iJrate = grd.getJrate();
    int iKrate = grd.getKrate();

    int newNI, newNJ, newNK;

    std::vector< std::vector<double> > resampledData = cg->getResampledValues( iIrate, iJrate, iKrate,
                                                                               newNI, newNJ, newNK);

    //make a tmp file path
    QString tmp_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("dat");

    //crate a new cartesian grid pointing to the tmp path
    CartesianGrid * new_cg = new CartesianGrid( tmp_file_path );

    //init the grid geometry info
    new_cg->setInfoFromOtherCG( cg );

    //adjust some grid geometry parameters to reflect the resampling
    double dx, dy, dz;
    dx = cg->getDX() * cg->getNX()/(double)newNI;
    dy = cg->getDY() * cg->getNY()/(double)newNJ;
    dz = cg->getDZ() * cg->getNZ()/(double)newNK;
    new_cg->setCellGeometry( newNI, newNJ, newNK, dx, dy, dz );

    //get the file's data GEO-EAS column names
    QStringList colNames = Util::getFieldNames( cg->getPath() );

    //get the file's GEO-EAS description (first text line)
    QString fileDescription = Util::getGEOEAScomment( cg->getPath() );

    //save the results in the project's tmp directory
    Util::createGEOEASGridFile(fileDescription, colNames.toVector().toStdVector(), resampledData, tmp_file_path);

    //import the saved file to the project
    Application::instance()->getProject()->importCartesianGrid( new_cg, new_cg_name );

    Application::instance()->logInfo("Grid resampling completed.");
}

void MainWindow::onMultiVariogram()
{
    QList<Attribute *> selectedAttributes = getSelectedAttributes();
    MultiVariogramDialog * mvd = new MultiVariogramDialog( selectedAttributes.toVector().toStdVector(), this );
    mvd->show(); //shows dialgo asynchronolously
}

void MainWindow::onHistpltsim()
{
    //get realizations attribute and attribute with reference distribution (if any)
    Attribute* realizationsAttribute = nullptr;
    Attribute* refDistAttribute = nullptr;
    CartesianGrid* grid = nullptr;
    PointSet* pointSet = nullptr;
    if( _right_clicked_attribute &&
        _right_clicked_attribute->getContainingFile()->getFileType() == "CARTESIANGRID" ){
        realizationsAttribute = _right_clicked_attribute;
        refDistAttribute = _right_clicked_attribute2;
    } else {
        realizationsAttribute = _right_clicked_attribute2;
        refDistAttribute = _right_clicked_attribute;
    }

    //get the files
    grid = (CartesianGrid*)realizationsAttribute->getContainingFile();
    if( refDistAttribute )
        pointSet = (PointSet*)refDistAttribute->getContainingFile();

    //load the data in grid
    grid->loadData();

    //get the maximum and minimun of selected variable, excluding no-data value
    double data_min = grid->min( realizationsAttribute->getAttributeGEOEASgivenIndex()-1 );
    double data_max = grid->max( realizationsAttribute->getAttributeGEOEASgivenIndex()-1 );
    data_min -= std::fabs( data_min / 100.0 );
    data_max += std::fabs( data_max / 100.0 );

    //------------------------------parameters setting--------------------------------------

    GSLibParameterFile gpf( "histpltsim" );

    //Set default values so we need to change less parameters and let
    //the user change the others as one may see fit.
    gpf.setDefaultValues();

    GSLibParMultiValuedFixed* par3 = gpf.getParameter<GSLibParMultiValuedFixed*>(3);
    if( refDistAttribute ){
        //file with reference distribution
        gpf.getParameter<GSLibParFile*>(2)->_path = pointSet->getPath();
        //   columns for reference variable and weight
        par3->getParameter<GSLibParUInt*>(0)->_value = refDistAttribute->getAttributeGEOEASgivenIndex();
        par3->getParameter<GSLibParUInt*>(1)->_value = 0;
    } else{
        gpf.getParameter<GSLibParFile*>(2)->_path = "NOFILE";
        par3->getParameter<GSLibParUInt*>(0)->_value = 0;
        par3->getParameter<GSLibParUInt*>(1)->_value = 0;
    }

    //file with distributions to check
    gpf.getParameter<GSLibParFile*>(4)->_path = grid->getPath();

    //   columns for variable and weight
    GSLibParMultiValuedFixed* par5 = gpf.getParameter<GSLibParMultiValuedFixed*>(5);
    par5->getParameter<GSLibParUInt*>(0)->_value = realizationsAttribute->getAttributeGEOEASgivenIndex();
    par5->getParameter<GSLibParUInt*>(1)->_value = 0;

    //   start and finish histograms (usually 1 and nreal)
    GSLibParMultiValuedFixed* par7 = gpf.getParameter<GSLibParMultiValuedFixed*>(7);
    par7->getParameter<GSLibParUInt*>(0)->_value = 1;
    par7->getParameter<GSLibParUInt*>(1)->_value = grid->getNReal();

    //   nx, ny, nz
    GSLibParMultiValuedFixed* par8 = gpf.getParameter<GSLibParMultiValuedFixed*>(8);
    par8->getParameter<GSLibParUInt*>(0)->_value = grid->getNX();
    par8->getParameter<GSLibParUInt*>(1)->_value = grid->getNY();
    par8->getParameter<GSLibParUInt*>(2)->_value = grid->getNZ();

    //   trimming limits
    GSLibParMultiValuedFixed* par9 = gpf.getParameter<GSLibParMultiValuedFixed*>(9);
    par9->getParameter<GSLibParDouble*>(0)->_value = data_min;
    par9->getParameter<GSLibParDouble*>(1)->_value = data_max;

    //file for PostScript output
    gpf.getParameter<GSLibParFile*>(10)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("ps");

    //file for summary output (always used)
    gpf.getParameter<GSLibParFile*>(11)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("txt");

    //file for numeric output (used if flag set above)
    gpf.getParameter<GSLibParFile*>(12)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("dat");;

    //attribute minimum and maximum
    GSLibParMultiValuedFixed* par13 = gpf.getParameter<GSLibParMultiValuedFixed*>(13);
    par13->getParameter<GSLibParDouble*>(0)->_value = data_min;
    par13->getParameter<GSLibParDouble*>(1)->_value = data_max;

    //title
    QString title = grid->getName() + "/" +
            realizationsAttribute->getName() + ": realizations";
    gpf.getParameter<GSLibParString*>(18)->_value = title;

    //reference value for box plot
    gpf.getParameter<GSLibParDouble*>(20)->_value = grid->mean( realizationsAttribute->getAttributeGEOEASgivenIndex()-1 );

    //----------------------------------------------------------------------------------

    //Generate the parameter file
    QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
    gpf.save( par_file_path );

    //run histpltsim program
    Application::instance()->logInfo("Starting histpltsim program...");
    GSLib::instance()->runProgram( "histpltsim", par_file_path );

    //display the plot output
    DisplayPlotDialog *dpd = new DisplayPlotDialog(gpf.getParameter<GSLibParFile*>(10)->_path, title, gpf, this);
    dpd->show(); //show() makes dialog modalless
}

void MainWindow::onRFFT()
{
    //propose a name for the new grid to contain the back tranformed image
    QString proposed_name = _right_clicked_attribute->getContainingFile()->getName() + "_RFFT.dat";

    //user enters the name for the new grid with back transformed image
    QString new_cg_name = QInputDialog::getText(this, "Name the new grid",
                                             "Name for the grid with back transformed image:", QLineEdit::Normal,
                                             proposed_name );

    //if the user canceled the input box
    if ( new_cg_name.isEmpty() ){
        //abort
        return;
    }

    //the parent file is surely a CartesianGrid.
    CartesianGrid *cg = (CartesianGrid*)_right_clicked_attribute->getContainingFile();

    //get the array containing the data (first variable is magnitude part (amplitude spectrum) and
    //the second is the angle part (phase spectrum)
    std::vector< std::complex<double> > array = cg->getArray( _right_clicked_attribute->getAttributeGEOEASgivenIndex()-1,
                                                              _right_clicked_attribute2->getAttributeGEOEASgivenIndex()-1);

    //run reverse FFT
    Util::fft3D( cg->getNX(),
                 cg->getNY(),
                 cg->getNZ(),
                 array,
                 FFTComputationMode::REVERSE,
                 FFTImageType::POLAR_FORM);

    //make a tmp file path
    QString tmp_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("dat");

    //crate a new cartesian grid pointing to the tmp path
    CartesianGrid * new_cg = new CartesianGrid( tmp_file_path );

    //set the geometry info based on the original grid
    new_cg->setInfoFromOtherCG( cg, false );

    //save the results in the project's tmp directory
    Util::createGEOEASGrid( "Real part", "Imaginary part", array, tmp_file_path);

    //import the saved file to the project
    Application::instance()->getProject()->importCartesianGrid( new_cg, new_cg_name );

    Application::instance()->logInfo("Reverse FFT completed.");
}

void MainWindow::onUpdateStatusBar()
{
    statusBar()->showMessage( "memory usage = " + Util::humanReadable( Util::getPhysicalRAMusage() ) + "B" );
}

void MainWindow::onCreateCategoryDefinition()
{
    CategoryDefinition *cd = new CategoryDefinition("");
    TriadsEditorDialog *ted = new TriadsEditorDialog( cd, this);
    ted->show();
}

void MainWindow::onClassifyInto()
{
    //assuming sender() returns a QAction* if execution passes through here.
    QAction *act = (QAction*)sender();

    //assuming the text in the menu item is the name of a categorical definition file.
    QString categoricalDefinitionFileName = act->text();

    //try to get the corresponding project component.
    ProjectComponent* pc = Application::instance()->getProject()->
            getResourcesGroup()->getChildByName( categoricalDefinitionFileName );
    if( ! pc ){
        Application::instance()->logError("MainWindow::onClassifyInto(): File " + categoricalDefinitionFileName +
                                          " not found in Resource Files group.");
        return;
    }

    //Assuming the project componente is a CategoryDefinition
    CategoryDefinition* cd = (CategoryDefinition*)pc;

    //Create a univariate classification table.
    m_ucc = new UnivariateCategoryClassification( cd, "" );

    //Open the dialog to edit the classification intervals and category.
    TriadsEditorDialog *ted = new TriadsEditorDialog( m_ucc, this );
    ted->setWindowTitle( "Classify " + _right_clicked_attribute->getName() + " into " + cd->getName() );
    ted->showOKbutton();
    connect( ted, SIGNAL(accepted()), this, SLOT(onPerformClassifyInto()));
    ted->show(); //show()->non-modal / execute()->modal
    //method onPerformClassifyInto() will be called upon dilog accept.
}

void MainWindow::onPerformClassifyInto()
{
    //Assumes the attribute's parent file is a DataFile
    DataFile* df = (DataFile*)_right_clicked_attribute->getContainingFile();

    //Get the target attribute GEO-EAS index in the data file
    uint index = df->getFieldGEOEASIndex( _right_clicked_attribute->getName() );

    //propose a name for the new variable
    QString proposed_name = m_ucc->getCategoryDefinition()->getName();

    //user enters the name for the new variable
    QString new_var_name = QInputDialog::getText(this, "Name the new categorical variable",
                                             "New variable name:", QLineEdit::Normal,
                                             proposed_name );

    //if the user didn't cancel the input box
    if ( ! new_var_name.isEmpty() ){
        //perfom de classification
        df->classify( index-1, m_ucc, new_var_name );
    }

    //TODO: delete m_ucc.
}

void MainWindow::createOrReviewVariogramModel(VariogramModel *vm)
{

    //Construct an object composition based on the parameter file template for the vmodel program.
    GSLibParameterFile gpf_vmodel( "vmodel" );

    QString title_complement;
    if( vm ){
        //if the user wants to review a model,
        //fill in the parameter file object with the contents of the saved vmodel parameter file
        gpf_vmodel.setValuesFromParFile( vm->getPath() );
        title_complement = ": " + vm->getName();
    }else{
        //if the user wants to create a model, then defaults are set
        gpf_vmodel.setDefaultValues();
    }

    //always generates a temp file path for the .var file for display with vargplt program
    //independently of what might have been saved to the vmodel par file
    gpf_vmodel.getParameter<GSLibParFile*>(0)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("out");

    //creates a parameter object for the vargplt program
    GSLibParameterFile gpf_vargplt( "vargplt" );

    //Set default values for vargplt so we need to change less parameters and let
    //the user change the others as one may see fit.
    gpf_vargplt.setDefaultValues();

    //----------------prepare the vargplt parameter file values for plotting-------------------
    //get the number of curves to be displayed from the vmodel parameter file
    GSLibParMultiValuedFixed *par1 = gpf_vmodel.getParameter<GSLibParMultiValuedFixed*>(1);
    uint ncurves_models = par1->getParameter<GSLibParUInt*>(0)->_value;

    //get the output file of the vmodel program
    QString vmodel_output_file = gpf_vmodel.getParameter<GSLibParFile*>(0)->_path;

    //make plot/window title
    QString title("Variogram Model" + title_complement );

    //postscript file
    gpf_vargplt.getParameter<GSLibParFile*>(0)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("ps");

    //suggest number of curves based on the variogram model parameters
    gpf_vargplt.getParameter<GSLibParUInt*>(1)->_value = ncurves_models; // nvarios

    //plot title
    gpf_vargplt.getParameter<GSLibParString*>(5)->_value = title;

    //suggest display settings for each variogram curve
    GSLibParRepeat *par6 = gpf_vargplt.getParameter<GSLibParRepeat*>(6); //repeat ndir-times
    par6->setCount( ncurves_models );
    //set display seetings for each curve
    for(uint i = 0; i < ncurves_models; ++i)
    {
        par6->getParameter<GSLibParFile*>(i, 0)->_path = vmodel_output_file;
        GSLibParMultiValuedFixed *par6_0_1 = par6->getParameter<GSLibParMultiValuedFixed*>(i, 1);
        par6_0_1->getParameter<GSLibParUInt*>(0)->_value = i + 1;
        par6_0_1->getParameter<GSLibParUInt*>(1)->_value = 0;
        par6_0_1->getParameter<GSLibParOption*>(2)->_selected_value = 0;
        par6_0_1->getParameter<GSLibParOption*>(3)->_selected_value = 1;
        par6_0_1->getParameter<GSLibParColor*>(4)->_color_code = (i % 15) + 1; //cycle through the available colors (except the gray tones)
    }

    //this loop allows parameter adjustment cycles
    while( true ){

        //show the parameter dialog so the user can review and adjust other settings before running vmodel
        GSLibParametersDialog *gslibpardiag = new GSLibParametersDialog( &gpf_vmodel );
        int result = gslibpardiag->exec();

        //aborts functionality if user clicks "Cancel" or presses "ESC" in the dialog
        if( result != QDialog::Accepted )
            return;

        //the user may have changed the number of variogram curves in vmodel parameters
        //so it is necessary to update the vargplt parameters accordingly
        GSLibParMultiValuedFixed *par1 = gpf_vmodel.getParameter<GSLibParMultiValuedFixed*>(1);
        ncurves_models = par1->getParameter<GSLibParUInt*>(0)->_value;
        gpf_vargplt.getParameter<GSLibParUInt*>(1)->_value = ncurves_models; // nvarios
        GSLibParRepeat *par6 = gpf_vargplt.getParameter<GSLibParRepeat*>(6); //repeat ndir-times
        par6->setCount( ncurves_models );

        //Generate the parameter file for vmodel program
        QString vmodel_par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
        gpf_vmodel.save( vmodel_par_file_path );

        //run vmodel program
        Application::instance()->logInfo("Starting vmodel program...");
        GSLib::instance()->runProgram( "vmodel", vmodel_par_file_path );

        //----------------------display plot------------------------------------------------------------

        //Generate the parameter file for vargplt program
        QString vargplt_par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
        gpf_vargplt.save( vargplt_par_file_path );

        //run vargplt program
        Application::instance()->logInfo("Starting vargplt program...");
        GSLib::instance()->runProgram( "vargplt", vargplt_par_file_path );

        //display the plot output
        DisplayPlotDialog *dpd = new DisplayPlotDialog(gpf_vargplt.getParameter<GSLibParFile*>(0)->_path,
                                                       gpf_vargplt.getParameter<GSLibParString*>(5)->_value,
                                                       gpf_vargplt,
                                                       this);
        result = dpd->exec();

        //ends the modeling loop if user clicks "OK" in the dialog
        if( result == QDialog::Accepted )
            break;
    }

    if( vm ){ //user is reviewing a model
        //asks whether the user wants to save any changes made to the variogram model
        uint result = QMessageBox::question(this, "Confirmation dialog.", "Save changes to the variogram model?",
                                      QMessageBox::Yes|QMessageBox::No);
        if (result == QMessageBox::Yes) {
            //save the variogram model values to the file corresponding to
            //variogram model file in the project directory
            gpf_vmodel.save( vm->getPath() );
        }
    }else{ //user wants to create a model
        //Generate the parameter file in the tmp directory as if we were about to run vmodel
        QString var_model_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
        gpf_vmodel.save( var_model_file_path );
        //propose a name for the file
        QString proposed_name( "new variogram model" );
        //open file rename dialog
        bool ok;
        QString new_var_model_name = QInputDialog::getText(this, "Name the new variogram model file",
                                                 "New variogram model file name:", QLineEdit::Normal,
                                                 proposed_name, &ok);
        if (ok && !new_var_model_name.isEmpty()){
            Application::instance()->getProject()->importVariogramModel( var_model_file_path, new_var_model_name.append(".vmodel") );
        }
    }
}

QList<Attribute *> MainWindow::getSelectedAttributes()
{
    //get the selected items' pointers
    QModelIndexList selected_indexes = ui->treeProject->selectionModel()->selectedIndexes();
    //prepare the list to return
    QList<Attribute*> result;
    //collect selected attributes
    for( int i = 0; i < selected_indexes.count(); ++i ){
        QModelIndex index = selected_indexes.at( i );
        ProjectComponent* pcAspect = static_cast<ProjectComponent*>( index.internalPointer() );
        if( pcAspect->isAttribute() ){
            result.append( (Attribute*)pcAspect );
        }
    }
    return result;
}

void MainWindow::makeMenuClassifyInto()
{
    m_subMenuClassifyInto->clear(); //remove any previously added item actions
    ObjectGroup* resources = Application::instance()->getProject()->getResourcesGroup();
    for( uint i = 0; i < (uint)resources->getChildCount(); ++i){
        ProjectComponent *pc = resources->getChildByIndex( i );
        if( pc->isFile() ){
            File *fileAspect = (File*)pc;
            if( fileAspect->getFileType() == "CATEGORYDEFINITION" ){
                m_subMenuClassifyInto->addAction( fileAspect->getIcon(),
                                                  fileAspect->getName(),
                                                  this,
                                                  SLOT(onClassifyInto()));
            }
        }
    }
}

void MainWindow::makeMenuClassifyWith()
{
    m_subMenuClassifyWith->clear(); //remove any previously added item actions
    ObjectGroup* resources = Application::instance()->getProject()->getResourcesGroup();
    for( uint i = 0; i < (uint)resources->getChildCount(); ++i){
        ProjectComponent *pc = resources->getChildByIndex( i );
        if( pc->isFile() ){
            File *fileAspect = (File*)pc;
            if( fileAspect->getFileType() == "UNIVARIATECATEGORYCLASSIFICATION" ){
                m_subMenuClassifyWith->addAction( fileAspect->getIcon(),
                                                  fileAspect->getName(),
                                                  this,
                                                  SLOT(onClassifyWith()));
            }
        }
    }
}

void MainWindow::makeMenuMapAs()
{
    m_subMenuMapAs->clear(); //remove any previously added item actions
    ObjectGroup* resources = Application::instance()->getProject()->getResourcesGroup();
    for( uint i = 0; i < (uint)resources->getChildCount(); ++i){
        ProjectComponent *pc = resources->getChildByIndex( i );
        if( pc->isFile() ){
            File *fileAspect = (File*)pc;
            if( fileAspect->getFileType() == "CATEGORYDEFINITION" ){
                m_subMenuMapAs->addAction( fileAspect->getIcon(),
                                                  fileAspect->getName(),
                                                  this,
                                                  SLOT(onMapAs()));
            }
        }
    }
}

void MainWindow::doAddDataFile(const QString filePath )
{
    if( ! filePath .isEmpty() && Application::instance()->hasOpenProject() ){
        Util::saveLastBrowsedDirectoryOfFile( filePath  );
        DataFileDialog dfd(this, filePath );
        dfd.exec();
        if( dfd.result() == QDialog::Accepted ){
            if( dfd.getDataFileType() == DataFileDialog::POINTSET ){
                PointSetDialog psd(this, filePath );
                psd.exec();
                if( psd.result() == QDialog::Accepted ){
                    //make the point set object
                    PointSet *ps = new PointSet( filePath  );
                    ps->setInfo( psd.getXFieldIndex(), psd.getYFieldIndex(), psd.getZFieldIndex(), psd.getNoDataValue() );
                    Application::instance()->getProject()->addDataFile( ps );
                }
            } else if( dfd.getDataFileType() == DataFileDialog::CARTESIANGRID ){
                CartesianGridDialog cgd(this, filePath );
                cgd.exec();
                if( cgd.result() == QDialog::Accepted ){
                    CartesianGrid *cg = new CartesianGrid( filePath  );
                    QMap<uint, QPair<uint, QString> > empty;
                    QList< QPair<uint,QString> > empty2;
                    cg->setInfo( cgd.getX0(), cgd.getY0(), cgd.getZ0(),
                                 cgd.getDX(), cgd.getDY(), cgd.getDZ(),
                                 cgd.getNX(), cgd.getNY(), cgd.getNZ(),
                                 cgd.getRot(), cgd.getNReal(), cgd.getNoDataValue(),
                                 empty, empty2 );
                    Application::instance()->getProject()->addDataFile( cg );
                }
            }
        }
    }
    this->refreshTreeStyle();
}

QString MainWindow::strippedName(const QString &fullDirPath)
{
    return QDir( fullDirPath ).dirName();
}

void MainWindow::showAbout(){
    AboutDialog ad(this);
    ad.exec();
}

void MainWindow::showSetup()
{
    SetupDialog sd(this);
    sd.exec();
}

void MainWindow::newProject()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select project directory", QDir::homePath());
    if( ! dir.isEmpty() ){
        Application::instance()->openProject( dir );
        this->setCurrentMRU( dir );
        if( Application::instance()->hasOpenProject() )
            ui->menuEstimation->setEnabled( true );
    }
    displayApplicationInfo();
}

void MainWindow::closeProject()
{
    Application::instance()->closeProject();
    displayApplicationInfo();
    ui->menuEstimation->setEnabled( false );
}

void MainWindow::openRecentProject()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
        openProject( action->data().toString() ); //action data contains complete path to project directory
}

void MainWindow::openKriging()
{
    KrigingDialog* kd = new KrigingDialog(this);
    kd->show();
}

void MainWindow::openIKContinuous()
{
    IndicatorKrigingDialog* ikd = new IndicatorKrigingDialog( IKVariableType::CONTINUOUS, this);
    ikd->show();
}

void MainWindow::openIKCategorical()
{
    IndicatorKrigingDialog* ikd = new IndicatorKrigingDialog( IKVariableType::CATEGORICAL, this);
    ikd->show();
}

void MainWindow::showMessagesConsoleCustomContextMenu(const QPoint &pt)
{
    QMenu *menu = ui->txtedMessages->createStandardContextMenu();
    menu->addAction("Clear", this, SLOT(onClearMessages()));
    menu->exec(ui->txtedMessages->mapToGlobal(pt));
    delete menu;
}

void MainWindow::openIKPostProcessing()
{
    PostikDialog* pd = new PostikDialog( this );
    pd->show();
}

void MainWindow::openCokriging()
{
    CokrigingDialog* cokd = new CokrigingDialog( this );
    cokd->show();
}

void MainWindow::openImageJockey()
{
    ImageJockeyDialog *ijd = new ImageJockeyDialog( this );
    //ijd->show();
    ijd->showMaximized();
}

void MainWindow::openSGSIM()
{
    SGSIMDialog *sgsd = new SGSIMDialog( this );
    sgsd->show();
}
