#include "communityfunddisplay.h"
#include "ui_communityfunddisplay.h"
#include "main.h"
#include <QtWidgets/QDialogButtonBox>
#include "../txdb.h"
#include <iomanip>
#include <sstream>

#include "consensus/cfund.h"
#include <iostream>

CommunityFundDisplay::CommunityFundDisplay(QWidget *parent, CFund::CProposal proposal) :
    QWidget(parent),
    ui(new Ui::CommunityFundDisplay),
    proposal(proposal)
{
    ui->setupUi(this);

    //connect buttons
    QList<QAbstractButton *> buttonBoxVoteButtonList = ui->buttonBoxVote->buttons();
    //this is not fetching correctly
    for(auto button : buttonBoxVoteButtonList)
        std::cout << button;
    connect(ui->buttonBoxVote, SIGNAL(clicked(buttonBoxVoteButtonList[0])), this, SLOT(on_click_buttonBoxVote(buttonBoxVoteButtonList[0])));
    connect(ui->buttonBoxVote, SIGNAL(clicked(buttonBoxVoteButtonList[1])), this, SLOT(on_click_buttonBoxVote(buttonBoxVoteButtonList[1])));


    //set labels from community fund
    ui->title->setText(QString::fromStdString(proposal.strDZeel));
    ui->labelStatus->setText(QString::fromStdString(proposal.GetState(pindexBestHeader->GetBlockTime())));
    stringstream n;
    n.imbue(std::locale(""));
    n << fixed << setprecision(8) << proposal.nAmount/100000000.0;
    string nav_amount = n.str();
    nav_amount.erase(nav_amount.find_last_not_of("0") + 1, std::string::npos );
    if(nav_amount.at(nav_amount.length()-1) == '.') {
        nav_amount = nav_amount.substr(0, nav_amount.size()-1);
    }
    nav_amount.append(" NAV");
    ui->labelRequested->setText(QString::fromStdString(nav_amount));

    //convert seconds to DD/HH/SS for proposal deadline
    //add expire time instead of deadline time
    //uint64_t deadline = proposal.getTimeTillExpired(chainActive.Tip()->GetBlockTime());

    uint64_t deadline = proposal.getTimeTillExpired(pindexBestHeader->GetBlockTime());
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

    //hide ui voting elements on proposals which are not allowed vote states
    if(!proposal.CanVote())
    {
        ui->buttonBoxVote->setStandardButtons(QDialogButtonBox::NoButton);
    }
}

void CommunityFundDisplay::on_click_buttonBoxVote(QAbstractButton *button)
{
    std::cout << "hello!\n";
    std::cout << button << "\n";
    return;
}

CommunityFundDisplay::~CommunityFundDisplay()
{
    delete ui;
}
