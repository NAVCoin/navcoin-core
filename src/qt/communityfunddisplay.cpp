#include "communityfunddisplay.h"
#include "ui_communityfunddisplay.h"
#include "main.h"
#include <QtWidgets/QDialogButtonBox>

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
    string nav_amount = std::to_string(proposal.nAmount/100000000.0);
    nav_amount.erase(nav_amount.find_last_not_of("0") + 1, std::string::npos );
    if(nav_amount.at(nav_amount.length()-1) == '.') {
        nav_amount = nav_amount.substr(0, nav_amount.size()-1);
    }
    nav_amount.append(" NAV");
    ui->labelRequested->setText(QString::fromStdString(nav_amount));

    //convert seconds to DD/HH/SS for proposal deadline
    uint64_t deadline = proposal.nDeadline;
    //add expire time instead of deadline time
    //uint64_t expiry = proposal.
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

    //set ui voting elements to not appear on proposals which are not allowed a vote state

    if(!proposal.CanVote())
    {
        ui->buttonBoxVote->setStandardButtons(QDialogButtonBox::NoButton);
    }
}

CommunityFundDisplay::~CommunityFundDisplay()
{
    delete ui;
}
