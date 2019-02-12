#include "communityfunddisplay.h"
#include "ui_communityfunddisplay.h"
#include "main.h"

CommunityFundDisplay::CommunityFundDisplay(QWidget *parent, CFund::CProposal proposal) :
    QWidget(parent),
    ui(new Ui::CommunityFundDisplay),
    proposal(proposal)
{
    ui->setupUi(this);

    //add a border
    //set labels from community fund
    ui->title->setText(QString::fromStdString(proposal.strDZeel));
    ui->labelStatus->setText(QString::fromStdString(proposal.GetState(pindexBestHeader->GetBlockTime())));
    ui->labelRequested->setText(QString::fromStdString(std::to_string(proposal.nAmount/100000000.0)));

    //convert seconds to DD/HH/SS for proposal deadline
    uint64_t deadline = proposal.nDeadline;
    uint64_t deadline_d = std::floor(deadline/86400);
    uint64_t deadline_h = std::floor((deadline-deadline_d*86400)/3600);
    uint64_t deadline_m = std::floor((deadline-(deadline_d*86400 + deadline_h*3600))/60);

    std::string s_deadline = "";
    if(deadline_d >= 14)
    {
        s_deadline = std::to_string(deadline_d) + std::string(" Days");
    }
    else
    {
        s_deadline = std::to_string(deadline_d) + std::string(" Days ") + std::to_string(deadline_h) + std::string(" Hours ") + std::to_string(deadline_m) + std::string(" Minutes");
    }

    ui->labelDuration->setText(QString::fromStdString(s_deadline));
}

CommunityFundDisplay::~CommunityFundDisplay()
{
    delete ui;
}
