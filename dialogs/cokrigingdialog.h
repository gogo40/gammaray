#ifndef COKRIGINGDIALOG_H
#define COKRIGINGDIALOG_H

#include <QDialog>
#include <QVector>

namespace Ui {
class CokrigingDialog;
}

class PointSetSelector;
class VariableSelector;
class CartesianGridSelector;
class QLabel;
class VariogramModelSelector;
class GSLibParameterFile;
class VariogramModel;
class CartesianGrid;

class CokrigingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CokrigingDialog(QWidget *parent = 0);
    ~CokrigingDialog();

private:
    Ui::CokrigingDialog *ui;
    PointSetSelector* m_psInputSelector;
    VariableSelector* m_inputPrimVarSelector;
    QVector<VariableSelector*> m_inputSecVarsSelectors;
    CartesianGridSelector* m_cgEstimationGridSelector;
    CartesianGridSelector* m_cgSecondaryGridSelector;
    QVector<VariableSelector*> m_inputGridSecVarsSelectors;
    QVector<QLabel*> m_labelsVarMatrixTopHeader;
    QVector<QLabel*> m_labelsVarMatrixLeftHeader;
    GSLibParameterFile* m_gpf_cokb3d;
    //first int = head variable order (1=primary, 2=1st secondary, ...), second int = tail variable order
    QVector< std::tuple<uint,uint,VariogramModelSelector*> > m_variograms;
    CartesianGrid* m_cg_estimation;

private slots:
    void onNumberOfSecondaryVariablesChanged( int n );
    void onUpdateVariogramMatrix( int numberOfSecondaryVariables );
    void onUpdateVarMatrixLabels();
    void onParameters();
    void onLMCcheck();
    void onCokb3dCompletes();
    void onSave();
    void onSaveKrigingVariances();

private:
    QLabel* makeLabel( const QString caption );
    VariableSelector* makeVariableSelector();
    VariogramModelSelector* makeVariogramModelSelector();
    /** Returns nullptr if the head/tail combination does not exist.
     * @note 1,3 == 3,1 due to assumed cross variogram symmetry (no lag effect).
     */
    VariogramModel *getVariogramModel( uint head, uint tail );
    void preview();
    void save( bool estimates );
};

#endif // COKRIGINGDIALOG_H
