#include "communityfundsuccessdialog.h"
#include "ui_communityfundsuccessdialog.h"
#include <QDialog>

CommunityFundSuccessDialog::CommunityFundSuccessDialog(QWidget *parent, CFund::CPaymentRequest* prequest) :
    QDialog(parent),
    ui(new Ui::CommunityFundSuccessDialog),
    proposal(0),
    prequest(prequest)
{
    ui->setupUi(this);
}

CommunityFundSuccessDialog::CommunityFundSuccessDialog(QWidget *parent, CFund::CProposal* proposal) :
    QDialog(parent),
    ui(new Ui::CommunityFundSuccessDialog),
    proposal(proposal),
    prequest(0)
{
    ui->setupUi(this);
}

CommunityFundSuccessDialog::~CommunityFundSuccessDialog()
{
    delete ui;
}
