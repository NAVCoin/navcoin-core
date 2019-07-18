// Copyright (c) 2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "daopage.h"

DaoPage::DaoPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    clientModel(0),
    walletModel(0),
    table(new QTableWidget),
    layout(new QVBoxLayout),
    contextMenu(new QMenu),
    fActive(false),
    fExclude(false),
    nView(VIEW_PROPOSALS),
    nCurrentView(-1),
    nCurrentUnit(DEFAULT_UNIT),
    nFilter(FILTER_ALL),
    nFilter2(FILTER_ALL),
    nCurrentFilter(FILTER_ALL),
    nCurrentFilter2(FILTER_ALL)
{
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setLayout(layout);

    auto *topBox = new QFrame;
    auto *topBoxLayout = new QHBoxLayout;
    topBoxLayout->setContentsMargins(QMargins());
    topBox->setLayout(topBoxLayout);

    auto *bottomBox = new QFrame;
    auto *bottomBoxLayout = new QHBoxLayout;
    bottomBoxLayout->setContentsMargins(QMargins());
    bottomBox->setLayout(bottomBoxLayout);

    viewLbl = new QLabel(tr("View:"));
    proposalsBtn = new NavCoinPushButton(tr("Fund Proposals"));
    paymentRequestsBtn = new NavCoinPushButton(tr("Payment Requests"));
    consultationsBtn = new NavCoinPushButton(tr("Consultations"));
    deploymentsBtn = new NavCoinPushButton(tr("Deployments"));
    consensusBtn = new NavCoinPushButton(tr("Consensus Parameters"));
    filterLbl = new QLabel(tr("Filter:"));
    filterCmb = new QComboBox();
    filter2Cmb = new QComboBox();
    createBtn = new QPushButton(tr("Create New"));
    excludeBox = new QCheckBox(tr("Exclude finished and expired"));
    childFilterLbl = new QLabel();
    backToFilterBtn = new QPushButton(tr("Back"));

    childFilterLbl->setVisible(false);
    backToFilterBtn->setVisible(false);

    connect(proposalsBtn, SIGNAL(clicked()), this, SLOT(viewProposals()));
    connect(paymentRequestsBtn, SIGNAL(clicked()), this, SLOT(viewPaymentRequests()));
    connect(consultationsBtn, SIGNAL(clicked()), this, SLOT(viewConsultations()));
    connect(deploymentsBtn, SIGNAL(clicked()), this, SLOT(viewDeployments()));
    connect(createBtn, SIGNAL(clicked()), this, SLOT(onCreate()));
    connect(filterCmb, SIGNAL(activated(int)), this, SLOT(onFilter(int)));
    connect(filter2Cmb, SIGNAL(activated(int)), this, SLOT(onFilter2(int)));
    connect(excludeBox, SIGNAL(toggled(bool)), this, SLOT(onExclude(bool)));
    connect(backToFilterBtn, SIGNAL(clicked()), this, SLOT(backToFilter()));

    excludeBox->setChecked(true);

    topBoxLayout->addSpacing(30);
    topBoxLayout->addWidget(viewLbl, 0, Qt::AlignLeft);
    topBoxLayout->addSpacing(30);
    topBoxLayout->addWidget(proposalsBtn);
    topBoxLayout->addWidget(paymentRequestsBtn);
    topBoxLayout->addWidget(consultationsBtn);
    topBoxLayout->addWidget(deploymentsBtn);
    topBoxLayout->addWidget(consensusBtn);
    topBoxLayout->addStretch();

    bottomBoxLayout->addSpacing(30);
    bottomBoxLayout->addWidget(filterLbl, 0, Qt::AlignLeft);
    bottomBoxLayout->addWidget(childFilterLbl);
    bottomBoxLayout->addWidget(backToFilterBtn);
    bottomBoxLayout->addSpacing(38);
    bottomBoxLayout->addWidget(filterCmb, 0, Qt::AlignLeft);
    bottomBoxLayout->addSpacing(30);
    bottomBoxLayout->addWidget(filter2Cmb, 0, Qt::AlignLeft);
    bottomBoxLayout->addSpacing(30);
    bottomBoxLayout->addWidget(excludeBox);
    bottomBoxLayout->addStretch();
    bottomBoxLayout->addWidget(createBtn);

    layout->addWidget(topBox);
    layout->addSpacing(15);
    layout->addWidget(table);
    layout->addWidget(bottomBox);

    table->setContentsMargins(QMargins());
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setFocusPolicy(Qt::NoFocus);
    table->setAlternatingRowColors(true);
    table->setShowGrid(false);
    table->setFocusPolicy(Qt::NoFocus);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    table->verticalHeader()->setDefaultSectionSize(78);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    table->horizontalHeader()->setSortIndicatorShown(true);
    table->horizontalHeader()->setSectionsClickable(true);
    table->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(table, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(showContextMenu(const QPoint &)));

    copyHash = new QAction(tr("Copy hash"), this);
    openExplorerAction = new QAction(tr("Open in explorer"), this);
    proposeAnswer = new QAction(tr("Propose Answer"), this);
    supportAnswer = new QAction(tr("Support Answers"), this);
    seeProposalAction = new QAction(tr("Go to parent proposal"), this);
    seePaymentRequestsAction = new QAction(tr("Show payment requests from this proposal"), this);

    contextMenu->addAction(copyHash);
    contextMenu->addAction(openExplorerAction);
    contextMenu->addSeparator();

    connect(proposeAnswer, SIGNAL(triggered()), this, SLOT(onProposeAnswer()));
    connect(supportAnswer, SIGNAL(triggered()), this, SLOT(onSupportAnswer()));
    connect(seeProposalAction, SIGNAL(triggered()), this, SLOT(onSeeProposal()));
    connect(seePaymentRequestsAction, SIGNAL(triggered()), this, SLOT(onSeePaymentRequests()));
    connect(copyHash, SIGNAL(triggered()), this, SLOT(onCopyHash()));
    connect(openExplorerAction, SIGNAL(triggered()), this, SLOT(onDetails()));

    viewProposals();
}

void DaoPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    refresh(true);
}

void DaoPage::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;

    if (clientModel) {
        connect(clientModel, SIGNAL(numBlocksChanged(int,QDateTime,double,bool)), this, SLOT(refreshForce()));
    }
}

void DaoPage::setActive(bool flag)
{
    fActive = flag;
}

void DaoPage::setView(int view)
{
    nView = view;

    refresh(true);
}

void DaoPage::refreshForce()
{
    refresh(true);
}

void DaoPage::refresh(bool force, bool updateFilterIfEmpty)
{
    CProposalMap proposalMap;
    CPaymentRequestMap paymentRequestMap;
    CConsultationMap consultationMap;
    CConsultationAnswerMap consultationAnswerMap;
    int unit = DEFAULT_UNIT;

    {
    LOCK(cs_main);

    if (pcoinsTip == nullptr)
        return;

    if (!pcoinsTip->GetAllProposals(proposalMap) || !pcoinsTip->GetAllPaymentRequests(paymentRequestMap) ||
            !pcoinsTip->GetAllConsultations(consultationMap) || !pcoinsTip->GetAllConsultationAnswers(consultationAnswerMap))
        return;

    if (clientModel)
    {
        OptionsModel* optionsModel = clientModel->getOptionsModel();

        if(optionsModel)
        {
            unit = optionsModel->getDisplayUnit();
        }
    }
    }

    if (!force && unit == nCurrentUnit && nFilter == nCurrentFilter && nFilter2 == nCurrentFilter2 &&
            ((nCurrentView == VIEW_PROPOSALS && proposalMap.size() == proposalModel.size()) ||
             (nCurrentView == VIEW_PAYMENT_REQUESTS &&  paymentRequestMap.size() == paymentRequestModel.size()) ||
             (nCurrentView == VIEW_CONSULTATIONS && consultationMap.size() == consultationModel.size())))
        return;

    initialize(proposalMap, paymentRequestMap, consultationMap, consultationAnswerMap, unit, updateFilterIfEmpty);
}

void DaoPage::initialize(CProposalMap proposalMap, CPaymentRequestMap paymentRequestMap, CConsultationMap consultationMap, CConsultationAnswerMap consultationAnswerMap, int unit, bool updateFilterIfEmpty)
{

    LOCK(cs_main);

    nCurrentUnit = unit;

    if (nView != nCurrentView)
    {
        while (filterCmb->count() > 0)
            filterCmb->removeItem(0);

        while (filter2Cmb->count() > 0)
            filter2Cmb->removeItem(0);

        nCurrentView = nView;
        nCurrentFilter = nFilter;
        nCurrentFilter2 = nFilter2;

        table->setRowCount(0);

        if (nCurrentView == VIEW_PROPOSALS)
        {
            createBtn->setVisible(true);
            createBtn->setText(tr("Create new Proposal"));

            filterCmb->insertItem(FILTER_ALL, "All");
            filterCmb->insertItem(FILTER_NOT_VOTED, "Not voted");
            filterCmb->insertItem(FILTER_VOTED, "Voted");
            filter2Cmb->insertItem(FILTER2_ALL, "All");
            filter2Cmb->insertItem(FILTER2_IN_PROGRESS, "Being voted");
            filter2Cmb->insertItem(FILTER2_FINISHED, "Voting finished");

            if (filterHash != "")
            {
                filterCmb->setVisible(false);
                filter2Cmb->setVisible(false);
                excludeBox->setVisible(false);
                backToFilterBtn->setVisible(true);
                childFilterLbl->setVisible(true);
                childFilterLbl->setText(tr("Showing proposal %1").arg(filterHash));
            }
            else
            {
                filterCmb->setVisible(true);
                filter2Cmb->setVisible(true);
                excludeBox->setVisible(true);
                backToFilterBtn->setVisible(false);
                childFilterLbl->setVisible(false);
            }

            table->setColumnCount(P_COLUMN_PADDING3 + 1);
            table->setColumnHidden(P_COLUMN_HASH, true);
            table->setHorizontalHeaderLabels({ "", "", tr("Name"), tr("Requests"), tr("Paid"), tr("Duration"), tr("Votes"), tr("State"), "", "", "" });
            table->horizontalHeader()->setSectionResizeMode(P_COLUMN_TITLE, QHeaderView::Stretch);
            table->horizontalHeaderItem(P_COLUMN_TITLE)->setTextAlignment(Qt::AlignLeft);
            table->horizontalHeader()->setSectionResizeMode(P_COLUMN_VOTE, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(P_COLUMN_REQUESTS, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(P_COLUMN_PAID, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(P_COLUMN_DURATION, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(P_COLUMN_VOTES, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(P_COLUMN_STATE, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(P_COLUMN_COLOR, QHeaderView::Fixed);
            table->setColumnWidth(P_COLUMN_COLOR, 12);
            table->horizontalHeader()->setSectionResizeMode(P_COLUMN_PADDING2, QHeaderView::Fixed);
            table->setColumnWidth(P_COLUMN_PADDING2, 12);
            table->horizontalHeader()->setSectionResizeMode(P_COLUMN_PADDING3, QHeaderView::Fixed);
            table->setColumnWidth(P_COLUMN_PADDING3, 12);
        }
        else if (nCurrentView == VIEW_PAYMENT_REQUESTS)
        {
            createBtn->setVisible(true);
            createBtn->setText(tr("Create new Payment Request"));

            filterCmb->insertItem(FILTER_ALL, "All");
            filterCmb->insertItem(FILTER_NOT_VOTED, "Not voted");
            filterCmb->insertItem(FILTER_VOTED, "Voted");
            filter2Cmb->insertItem(FILTER2_ALL, "All");
            filter2Cmb->insertItem(FILTER2_IN_PROGRESS, "Being voted");
            filter2Cmb->insertItem(FILTER2_FINISHED, "Voting finished");

            if (filterHash != "")
            {
                filterCmb->setVisible(false);
                filter2Cmb->setVisible(false);
                excludeBox->setVisible(false);
                backToFilterBtn->setVisible(true);
                childFilterLbl->setVisible(true);
                childFilterLbl->setText(tr("Showing payment requests of %1").arg(filterHash));
            }
            else
            {
                filterCmb->setVisible(true);
                filter2Cmb->setVisible(true);
                excludeBox->setVisible(true);
                backToFilterBtn->setVisible(false);
                childFilterLbl->setVisible(false);
            }

            table->setColumnCount(PR_COLUMN_PADDING3 + 1);
            table->setColumnHidden(PR_COLUMN_HASH, true);
            table->setHorizontalHeaderLabels({ "", "", tr("Proposal"), tr("Description"), tr("Requests"), tr("Votes"), tr("State"), "", "", ""});
            table->horizontalHeader()->setSectionResizeMode(PR_COLUMN_PARENT_TITLE, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(PR_COLUMN_TITLE, QHeaderView::Stretch);
            table->horizontalHeaderItem(PR_COLUMN_PARENT_TITLE)->setTextAlignment(Qt::AlignLeft);
            table->horizontalHeader()->setSectionResizeMode(PR_COLUMN_REQUESTS, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(PR_COLUMN_VOTES, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(PR_COLUMN_STATE, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(PR_COLUMN_COLOR, QHeaderView::Fixed);
            table->horizontalHeader()->setSectionResizeMode(PR_COLUMN_VOTE, QHeaderView::ResizeToContents);
            table->setColumnWidth(PR_COLUMN_COLOR, 12);
            table->horizontalHeader()->setSectionResizeMode(PR_COLUMN_PADDING2, QHeaderView::Fixed);
            table->setColumnWidth(PR_COLUMN_PADDING2, 12);
            table->horizontalHeader()->setSectionResizeMode(PR_COLUMN_PADDING3, QHeaderView::Fixed);
            table->setColumnWidth(PR_COLUMN_PADDING3, 12);
        }
        else if (nCurrentView == VIEW_CONSULTATIONS)
        {
            createBtn->setVisible(true);
            createBtn->setText(tr("Create new Consultation"));

            filterCmb->insertItem(FILTER_ALL, "All");
            filterCmb->insertItem(FILTER_NOT_VOTED, "Not voted");
            filterCmb->insertItem(FILTER_VOTED, "Voted");
            filter2Cmb->insertItem(FILTER2_ALL, "All");
            filter2Cmb->insertItem(FILTER2_IN_PROGRESS, "Being voted");
            filter2Cmb->insertItem(FILTER2_FINISHED, "Voting finished");
            filter2Cmb->insertItem(FILTER2_LOOKING_FOR_SUPPORT, "Looking for support");

            table->setColumnCount(C_COLUMN_PADDING3 + 1);
            table->setColumnHidden(C_COLUMN_HASH, true);
            table->setHorizontalHeaderLabels({ "", "", tr("Question"), tr("Possible Answers"), tr("Status"), "", tr("My Votes"), "", ""});
            table->horizontalHeader()->setSectionResizeMode(C_COLUMN_TITLE, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(C_COLUMN_ANSWERS, QHeaderView::Stretch);
            table->horizontalHeaderItem(C_COLUMN_TITLE)->setTextAlignment(Qt::AlignLeft);
            table->horizontalHeader()->setSectionResizeMode(C_COLUMN_STATUS, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(C_COLUMN_MY_VOTES, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(C_COLUMN_COLOR, QHeaderView::Fixed);
            table->horizontalHeader()->setSectionResizeMode(C_COLUMN_VOTE, QHeaderView::ResizeToContents);
            table->setColumnWidth(C_COLUMN_COLOR, 12);
            table->horizontalHeader()->setSectionResizeMode(C_COLUMN_PADDING2, QHeaderView::Fixed);
            table->setColumnWidth(C_COLUMN_PADDING2, 12);
            table->horizontalHeader()->setSectionResizeMode(C_COLUMN_PADDING3, QHeaderView::Fixed);
            table->setColumnWidth(C_COLUMN_PADDING3, 12);
        }
        else if (nCurrentView == VIEW_DEPLOYMENTS)
        {
            createBtn->setVisible(false);

            filterCmb->insertItem(FILTER_ALL, "All");
            filterCmb->insertItem(FILTER_NOT_VOTED, "Not voted");
            filterCmb->insertItem(FILTER_VOTED, "Voted");
            filter2Cmb->insertItem(FILTER2_ALL, "All");
            filter2Cmb->insertItem(FILTER2_IN_PROGRESS, "Being voted");
            filter2Cmb->insertItem(FILTER2_FINISHED, "Voting finished");

            table->setColumnCount(D_COLUMN_PADDING3 + 1);
            table->setColumnHidden(D_COLUMN_COLOR, false);
            table->setHorizontalHeaderLabels({"", tr("Name"), tr("Status"), "", tr("My Vote"), "", "" });
            table->horizontalHeader()->setSectionResizeMode(D_COLUMN_TITLE, QHeaderView::Stretch);
            table->horizontalHeaderItem(D_COLUMN_TITLE)->setTextAlignment(Qt::AlignLeft);
            table->horizontalHeader()->setSectionResizeMode(D_COLUMN_STATUS, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(D_COLUMN_MY_VOTES, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(D_COLUMN_VOTE, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(D_COLUMN_COLOR, QHeaderView::Fixed);
            table->setColumnWidth(D_COLUMN_COLOR, 12);
            table->horizontalHeader()->setSectionResizeMode(D_COLUMN_PADDING2, QHeaderView::Fixed);
            table->setColumnWidth(D_COLUMN_PADDING2, 12);
            table->horizontalHeader()->setSectionResizeMode(D_COLUMN_PADDING3, QHeaderView::Fixed);
            table->setColumnWidth(D_COLUMN_PADDING3, 12);
        }

        onFilter(FILTER_ALL);
    }

    CStateViewCache coins(pcoinsTip);

    nBadgeProposals = nBadgePaymentRequests = nBadgeConsultations = nBadgeDeployments = 0;

    proposalModel.clear();

    for (auto& it: proposalMap)
    {
        CProposal proposal = it.second;

        uint64_t nVote = -10000;
        auto v = mapAddedVotes.find(proposal.hash);

        if (v != mapAddedVotes.end())
        {
            nVote = v->second;
        }

        if (proposal.CanVote() && nVote == -10000)
            nBadgeProposals++;

        if (!filterHash.isEmpty())
        {
            if (proposal.hash != uint256S(filterHash.toStdString()))
                continue;
        }
        else
        {
            if (nFilter != FILTER_ALL)
            {
                if (nFilter == FILTER_VOTED && nVote == -10000)
                    continue;
                if (nFilter == FILTER_NOT_VOTED && nVote != -10000)
                    continue;
            }
            if (nFilter2 != FILTER2_ALL)
            {
                if (nFilter2 == FILTER2_IN_PROGRESS && !proposal.CanVote())
                    continue;
                if (nFilter2 == FILTER2_FINISHED && proposal.CanVote())
                    continue;
            }
        }

        if (fExclude && proposal.IsExpired(chainActive.Tip()->GetBlockTime()))
            continue;

        if (mapBlockIndex.count(proposal.txblockhash) == 0)
            continue;

        uint64_t deadline = proposal.nDeadline;
        uint64_t deadline_d = std::floor(deadline/86400);
        uint64_t deadline_h = std::floor((deadline-deadline_d*86400)/3600);
        uint64_t deadline_m = std::floor((deadline-(deadline_d*86400 + deadline_h*3600))/60);

        // Show appropriate amount of figures
        std::string s_deadline = "";
        if(deadline_d >= 14)
            s_deadline = std::to_string(deadline_d) + std::string(" Days");
        else
            s_deadline = std::to_string(deadline_d) + std::string(" Days ") + std::to_string(deadline_h) + std::string(" Hours ") + std::to_string(deadline_m) + std::string(" Minutes");

        ProposalEntry p = {
            it.first,
            "#6666ff",
            QString::fromStdString(proposal.strDZeel),
            NavCoinUnits::formatWithUnit(unit, proposal.nAmount, false, NavCoinUnits::separatorAlways),
            NavCoinUnits::formatWithUnit(unit, proposal.nAmount - proposal.GetAvailable(coins), false, NavCoinUnits::separatorAlways),
            QString::fromStdString(s_deadline),
            proposal.nVotesYes ? proposal.nVotesYes : 0,
            proposal.nVotesNo ? proposal.nVotesNo : 0,
            proposal.nVotesAbs ? proposal.nVotesAbs : 0,
            proposal.nVotingCycle,
            QString::fromStdString(proposal.GetState(chainActive.Tip()->GetBlockTime())),
            proposal.CanVote(),
            nVote,
            (uint64_t)mapBlockIndex[proposal.txblockhash]->GetBlockTime()
        };

        switch (proposal.fState)
        {
        case DAOFlags::NIL:
        {
            p.color = "#ffdd66";
            break;
        }
        case DAOFlags::ACCEPTED:
        {
            p.color = "#66ff66";
            break;
        }
        case DAOFlags::REJECTED:
        {
            p.color = "#ff6666";
            break;
        }
        case DAOFlags::EXPIRED:
        {
            p.color = "#ff6600";
            break;
        }
        }

        proposalModel << p;
    }

    std::sort(proposalModel.begin(), proposalModel.end(), [](const ProposalEntry &a, const ProposalEntry &b) {
        if (a.ts == b.ts)
            return a.hash > b.hash;
        else
            return a.ts > b.ts;
    });

    paymentRequestModel.clear();

    for (auto& it: paymentRequestMap)
    {
        CProposal proposal;
        CPaymentRequest prequest = it.second;

        uint64_t nVote = -10000;
        auto v = mapAddedVotes.find(prequest.hash);

        if (v != mapAddedVotes.end())
        {
            nVote = v->second;
        }

        if (prequest.CanVote(coins) && nVote == -10000)
            nBadgePaymentRequests++;

        if (!filterHash.isEmpty())
        {
            if (prequest.proposalhash != uint256S(filterHash.toStdString()))
                continue;
        }
        else
        {
            if (nFilter != FILTER_ALL)
            {
                if (nFilter == FILTER_VOTED && nVote == -10000)
                    continue;
                if (nFilter == FILTER_NOT_VOTED && nVote != -10000)
                    continue;
            }

            if (nFilter2 != FILTER2_ALL)
            {
                if (nFilter2 == FILTER2_IN_PROGRESS && !prequest.CanVote(coins))
                    continue;
                if (nFilter2 == FILTER2_FINISHED && prequest.CanVote(coins))
                    continue;
            }
        }

        if (fExclude && prequest.IsExpired())
            continue;

        if (mapBlockIndex.count(prequest.txblockhash) == 0)
            continue;

        if (!coins.GetProposal(prequest.proposalhash, proposal))
            continue;

        PaymentRequestEntry p = {
            it.first,
            QString::fromStdString(prequest.strDZeel),
            QString::fromStdString(proposal.strDZeel),
            "#6666ff",
            NavCoinUnits::formatWithUnit(unit, prequest.nAmount, false, NavCoinUnits::separatorAlways),
            prequest.nVotesYes ? prequest.nVotesYes : 0,
            prequest.nVotesNo ? prequest.nVotesNo : 0,
            prequest.nVotesAbs ? prequest.nVotesAbs : 0,
            proposal.nVotingCycle,
            QString::fromStdString(prequest.GetState()),
            prequest.CanVote(coins),
            nVote,
            (uint64_t)mapBlockIndex[prequest.txblockhash]->GetBlockTime()
        };

        switch (prequest.fState)
        {
        case DAOFlags::NIL:
        {
            p.color = "#ffdd66";
            break;
        }
        case DAOFlags::ACCEPTED:
        {
            p.color = "#66ff66";
            break;
        }
        case DAOFlags::REJECTED:
        {
            p.color = "#ff6666";
            break;
        }
        case DAOFlags::EXPIRED:
        {
            p.color = "#ff6600";
            break;
        }
        }

        paymentRequestModel << p;
    }

    std::sort(paymentRequestModel.begin(), paymentRequestModel.end(), [](const PaymentRequestEntry &a, const PaymentRequestEntry &b) {
        if (a.ts == b.ts)
            return a.hash > b.hash;
        else
            return a.ts > b.ts;
    });

    consultationModel.clear();

    for (auto& it: consultationMap)
    {
        CConsultation consultation = it.second;

        if (mapBlockIndex.count(consultation.txblockhash) == 0)
            continue;

        uint64_t nVote = -10000;
        auto v = mapAddedVotes.find(consultation.hash);

        if (v != mapAddedVotes.end())
        {
            nVote = v->second;
        }

        QVector<ConsultationAnswerEntry> answers;
        QStringList myVotes;

        if (!consultation.IsRange())
        {
            for (auto& it2: consultationAnswerMap)
            {
                CConsultationAnswer answer;

                if (!pcoinsTip->GetConsultationAnswer(it2.first, answer))
                    continue;

                if (answer.parent != consultation.hash)
                    continue;

                if (!answer.IsSupported() && consultation.fState != DAOFlags::NIL)
                    continue;

                if (!consultation.IsRange())
                {
                    auto v = mapAddedVotes.find(answer.hash);

                    if (v != mapAddedVotes.end() && v->second == 1)
                    {
                        myVotes << QString::fromStdString(answer.sAnswer);
                    }
                }

                ConsultationAnswerEntry a = {
                    answer.hash,
                    QString::fromStdString(answer.sAnswer),
                    answer.nVotes,
                    QString::fromStdString(answer.GetState()),
                    answer.CanBeVoted(coins),
                    answer.CanBeSupported(coins),
                    answer.CanBeSupported(coins) && mapSupported.count(answer.hash) ? tr("Supported") : tr("")
                };

                answers << a;
            }
        }

        if (consultation.fState == DAOFlags::ACCEPTED &&
          ((nVote == -10000 && consultation.IsRange()) || (!consultation.IsRange() && myVotes.size() == 0)))
            nBadgeConsultations++;

        if (nFilter != FILTER_ALL)
        {
            if (nFilter == FILTER_VOTED && nVote == -10000 && myVotes.size() == 0)
                continue;
            if (nFilter == FILTER_NOT_VOTED && (nVote != -10000 || myVotes.size() > 0))
                continue;
        }
        if (nFilter2 != FILTER2_ALL)
        {
            if (nFilter2 == FILTER2_IN_PROGRESS && consultation.fState != DAOFlags::ACCEPTED)
                continue;
            if (nFilter2 == FILTER2_FINISHED && consultation.fState != DAOFlags::EXPIRED)
                continue;
            if ((consultation.CanBeSupported() && nFilter2 != FILTER2_LOOKING_FOR_SUPPORT) ||
                (!consultation.CanBeSupported() && nFilter2 == FILTER2_LOOKING_FOR_SUPPORT))
                continue;
        }

        if (fExclude && consultation.IsExpired(chainActive.Tip()))
            continue;

        bool fSupported = mapSupported.count(consultation.hash);

        if (consultation.CanBeSupported())
        {
            if (fSupported)
                myVotes << tr("Supported");
        }
        else if (consultation.IsRange() && nVote != -10000)
        {
            myVotes << QString::number(nVote);
        }

        if (consultation.IsRange())
        {
            std::sort(answers.begin(), answers.end(), [](const ConsultationAnswerEntry &a, const ConsultationAnswerEntry &b) {
                return a.answer.toInt() < b.answer.toInt();
            });
        }
        else
        {
            std::sort(answers.begin(), answers.end(), [](const ConsultationAnswerEntry &a, const ConsultationAnswerEntry &b) {
                return a.answer.toInt() < b.answer.toInt();
            });
        }

        myVotes.sort();

        bool fCanVote = consultation.CanBeVoted();

        if (!consultation.IsRange())
        {
            fCanVote = true;

            for (ConsultationAnswerEntry &a: answers)
            {
                if (!a.fCanVote)
                {
                    fCanVote = false;
                    break;
                }
            }
        }

        ConsultationEntry p = {
            it.first,
            "#6666ff",
            QString::fromStdString(consultation.strDZeel),
            answers,
            "",
            consultation.nVotingCycle,
            QString::fromStdString(consultation.GetState(chainActive.Tip())),
            consultation.fState,
            fCanVote,
            myVotes,
            (uint64_t)mapBlockIndex[consultation.txblockhash]->GetBlockTime(),
            consultation.IsRange(),
            consultation.nMin,
            consultation.nMax,
            consultation.CanBeSupported()
        };

        switch (consultation.fState)
        {
        case DAOFlags::NIL:
        {
            p.color = "#ffdd66";
            break;
        }
        case DAOFlags::ACCEPTED:
        {
            p.color = "#66ff66";
            break;
        }
        case DAOFlags::EXPIRED:
        {
            p.color = "#ff6666";
            break;
        }
        case DAOFlags::CONFIRMATION:
        {
            p.color = "#ff6600";
            break;
        }
        }

        consultationModel << p;
    }

    std::sort(consultationModel.begin(), consultationModel.end(), [](const ConsultationEntry &a, const ConsultationEntry &b) {
        if (a.ts == b.ts)
            return a.hash > b.hash;
        else
            return a.ts > b.ts;
    });

    deploymentModel.clear();

    const Consensus::Params& consensusParams = Params().GetConsensus();

    for (unsigned int i = 0; i < Consensus::MAX_VERSION_BITS_DEPLOYMENTS; i++)
    {
        QString status = "unknown";

        Consensus::DeploymentPos id = (Consensus::DeploymentPos)i;

        bool fFinished = false;
        bool fInProgress = false;
        bool nVoted = false;

        if (!consensusParams.vDeployments[id].nStartTime)
            continue;

        std::vector<std::string>& versionBitVotesRejected = mapMultiArgs["-rejectversionbit"];
        std::vector<std::string>& versionBitVotesAccepted = mapMultiArgs["-acceptversionbit"];

        for (auto& it: versionBitVotesRejected)
        {
            if (stoi(it) == i)
            {
                nVoted = true;
                break;
            }
        }

        if (!nVoted)
        {
            for (auto& it: versionBitVotesAccepted)
            {
                if (stoi(it) == i)
                {
                    nVoted = true;
                    break;
                }
            }
        }

        const ThresholdState thresholdState = VersionBitsTipState(consensusParams, id);
        switch (thresholdState) {
        case THRESHOLD_DEFINED: status =  "defined"; fFinished = false; fInProgress = false; break;
        case THRESHOLD_STARTED: status = "started"; fFinished = false; fInProgress = true; break;
        case THRESHOLD_LOCKED_IN: status = "locked in"; fFinished = true; fInProgress = false; break;
        case THRESHOLD_ACTIVE: status = "active"; fFinished = true; fInProgress = false; break;
        case THRESHOLD_FAILED: status = "failed"; fFinished = true; fInProgress = false; break;
        }

        if (fInProgress && !nVoted)
            nBadgeDeployments++;

        if (nFilter != FILTER_ALL)
        {
            if (nFilter == FILTER_VOTED && !nVoted)
                continue;
            if (nFilter == FILTER_NOT_VOTED && nVoted)
                continue;
        }
        if (nFilter2 != FILTER2_ALL)
        {
            if (nFilter2 == FILTER2_IN_PROGRESS && !fInProgress)
                continue;
            if (nFilter2 == FILTER2_FINISHED && !fFinished)
                continue;
        }

        bool nVote = !IsVersionBitRejected(consensusParams, id);

        DeploymentEntry e = {
            "#6666ff",
            QString::fromStdString(Consensus::sDeploymentsDesc[id]),
            status,
            status == "started",
            nVote,
            !nVoted,
            consensusParams.vDeployments[id].nStartTime,
            consensusParams.vDeployments[id].bit
        };

        if (status == "started")
        {
            e.color = "#ffdd66";
        }
        else if (status == "locked_in")
        {
            e.color = "#ff66ff";
        }
        else if (status == "active")
        {
            e.color = "#66ff66";
        }
        else if (status == "failed")
        {
            e.color = "#ff6666";
        }

        deploymentModel << e;
    }

    std::sort(deploymentModel.begin(), deploymentModel.end(), [](const DeploymentEntry &a, const DeploymentEntry &b) {
        if (a.ts == b.ts)
            return a.title > b.title;
        else
            return a.ts > b.ts;
    });

    if (nCurrentView == VIEW_PROPOSALS)
    {
        if (updateFilterIfEmpty && proposalModel.size() == 0)
            onFilter(FILTER_VOTED);
        setData(proposalModel);
    }
    else if (nCurrentView == VIEW_PAYMENT_REQUESTS)
    {
        if (updateFilterIfEmpty && paymentRequestModel.size() == 0)
            onFilter(FILTER_VOTED);
        setData(paymentRequestModel);
    }
    else if (nCurrentView == VIEW_CONSULTATIONS)
    {
        if (updateFilterIfEmpty && consultationModel.size() == 0)
            onFilter(FILTER_VOTED);
        setData(consultationModel);
    }
    else if (nCurrentView == VIEW_DEPLOYMENTS)
    {
        if (updateFilterIfEmpty && deploymentModel.size() == 0)
            onFilter(FILTER_VOTED);
        setData(deploymentModel);
    }

    proposalsBtn->setBadge(nBadgeProposals);
    paymentRequestsBtn->setBadge(nBadgePaymentRequests);
    consultationsBtn->setBadge(nBadgeConsultations);
    deploymentsBtn->setBadge(nBadgeDeployments);
}

void DaoPage::setData(QVector<ProposalEntry> data)
{
    table->clearContents();
    table->setRowCount(data.count());
    table->setSortingEnabled(false);

    for (int i = 0; i < data.count(); ++i) {
        auto &entry = data[i];

        // HASH
        auto *hashItem = new QTableWidgetItem;
        hashItem->setData(Qt::DisplayRole, QString::fromStdString(entry.hash.GetHex()));
        table->setItem(i, P_COLUMN_HASH, hashItem);

        // COLOR
        auto *colorItem = new QTableWidgetItem;
        auto *indicatorBox = new QFrame;
        indicatorBox->setContentsMargins(QMargins());
        indicatorBox->setStyleSheet(QString("background-color: %1;").arg(entry.color));
        indicatorBox->setFixedWidth(3);
        table->setCellWidget(i, P_COLUMN_COLOR, indicatorBox);
        table->setItem(i, P_COLUMN_COLOR, colorItem);

        // TITLE
        auto *nameItem = new QTableWidgetItem;
        nameItem->setData(Qt::DisplayRole, entry.title);
        table->setItem(i, P_COLUMN_TITLE, nameItem);

        // AMOUNT
        auto *amountItem = new QTableWidgetItem;
        amountItem->setData(Qt::DisplayRole, entry.requests);
        amountItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, P_COLUMN_REQUESTS, amountItem);

        // PAID
        auto *paidItem = new QTableWidgetItem;
        paidItem->setData(Qt::DisplayRole, entry.paid);
        paidItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, P_COLUMN_PAID, paidItem);

        // DEADLINE
        auto *deadlineItem = new QTableWidgetItem;
        deadlineItem->setData(Qt::DisplayRole, entry.deadline);
        deadlineItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, P_COLUMN_DURATION, deadlineItem);

        // VOTES
        auto *votesItem = new QTableWidgetItem;
        votesItem->setData(Qt::DisplayRole, QString::number(entry.nVotesYes) + "/" + QString::number(entry.nVotesNo) + "/" + QString::number(entry.nVotesAbs));
        votesItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, P_COLUMN_VOTES, votesItem);

        // STATE
        auto *stateItem = new QTableWidgetItem;
        stateItem->setData(Qt::DisplayRole, entry.sState);
        stateItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, P_COLUMN_STATE, stateItem);

        //VOTE
        auto *votedItem = new QTableWidgetItem;
        auto *widget = new QWidget();
        widget->setContentsMargins(QMargins());
        widget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        auto *boxLayout = new QHBoxLayout;
        boxLayout->setContentsMargins(QMargins());
        boxLayout->setSpacing(0);
        widget->setLayout(boxLayout);

        auto voteText = QString();

        if (entry.nMyVote == 0)
        {
            voteText = tr("Voted No.");
        }
        else if (entry.nMyVote == 1)
        {
            voteText = tr("Voted Yes.");
        }
        else if (entry.nMyVote == -1)
        {
            voteText = tr("Abstaining.");
        }

        // If we've already voted, display a label with vote status above the vote button
        auto *voteLbl = new QLabel;
        if (!voteText.isEmpty()) {
            voteLbl->setText(voteText);
            boxLayout->addWidget(voteLbl, 0, Qt::AlignCenter);
            boxLayout->addSpacing(5);
        } else if (entry.fCanVote) {
            voteLbl->setText(tr("Did not vote."));
            boxLayout->addWidget(voteLbl, 0, Qt::AlignCenter);
            boxLayout->addSpacing(5);
        } else {
            voteLbl->setText(tr("Voting finished."));
            boxLayout->addWidget(voteLbl, 0, Qt::AlignCenter);
            boxLayout->addSpacing(5);
        }

        // Only show vote button if proposal voting is in progress
        if (entry.fCanVote) {
            auto *button = new QPushButton;
            button->setText(voteText.isEmpty() ? tr("Vote") : tr("Change"));
            button->setFixedHeight(40);
            button->setProperty("id", QString::fromStdString(entry.hash.GetHex()));
            boxLayout->addWidget(button, 0, Qt::AlignCenter);
            connect(button, SIGNAL(clicked()), this, SLOT(onVote()));
        }

        table->setCellWidget(i, P_COLUMN_VOTE, widget);
        table->setItem(i, P_COLUMN_VOTE, votedItem);
    }

    uint64_t nWeight = 0;
    if (pwalletMain)
        nWeight = pwalletMain->GetStakeWeight();
    bool fWeight = nWeight > 0;
    if (!fWeight) table->setColumnWidth(P_COLUMN_VOTE, 0);
    else table->setColumnWidth(P_COLUMN_VOTE, 150);
    table->setColumnHidden(P_COLUMN_VOTE, !fWeight);
    table->setSortingEnabled(true);

}

void DaoPage::setData(QVector<PaymentRequestEntry> data)
{
    table->clearContents();
    table->setRowCount(data.count());
    table->setSortingEnabled(false);

    for (int i = 0; i < data.count(); ++i) {
        auto &entry = data[i];

        // HASH
        auto *hashItem = new QTableWidgetItem;
        hashItem->setData(Qt::DisplayRole, QString::fromStdString(entry.hash.GetHex()));
        table->setItem(i, PR_COLUMN_HASH, hashItem);

        // COLOR
        auto *colorItem = new QTableWidgetItem;
        auto *indicatorBox = new QFrame;
        indicatorBox->setContentsMargins(QMargins());
        indicatorBox->setStyleSheet(QString("background-color: %1;").arg(entry.color));
        indicatorBox->setFixedWidth(3);
        table->setCellWidget(i, PR_COLUMN_COLOR, indicatorBox);
        table->setItem(i, PR_COLUMN_COLOR, colorItem);

        // PARENT TITLE
        auto *parentNameItem = new QTableWidgetItem;
        parentNameItem->setData(Qt::DisplayRole, entry.parentTitle);
        table->setItem(i, PR_COLUMN_PARENT_TITLE, parentNameItem);

        // TITLE
        auto *nameItem = new QTableWidgetItem;
        nameItem->setData(Qt::DisplayRole, entry.title);
        nameItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, PR_COLUMN_TITLE, nameItem);

        // AMOUNT
        auto *amountItem = new QTableWidgetItem;
        amountItem->setData(Qt::DisplayRole, entry.amount);
        amountItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, PR_COLUMN_REQUESTS, amountItem);

        // VOTES
        auto *votesItem = new QTableWidgetItem;
        votesItem->setData(Qt::DisplayRole, QString::number(entry.nVotesYes) + "/" + QString::number(entry.nVotesNo) + "/" + QString::number(entry.nVotesAbs));
        votesItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, PR_COLUMN_VOTES, votesItem);

        // STATE
        auto *stateItem = new QTableWidgetItem;
        stateItem->setData(Qt::DisplayRole, entry.sState);
        stateItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, PR_COLUMN_STATE, stateItem);

        //VOTE
        auto *votedItem = new QTableWidgetItem;
        auto *widget = new QWidget();
        widget->setContentsMargins(QMargins());
        widget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        auto *boxLayout = new QHBoxLayout;
        boxLayout->setContentsMargins(QMargins());
        boxLayout->setSpacing(0);
        widget->setLayout(boxLayout);

        auto voteText = QString();

        if (entry.nMyVote == 0)
        {
            voteText = tr("Voted No.");
        }
        else if (entry.nMyVote == 1)
        {
            voteText = tr("Voted Yes.");
        }
        else if (entry.nMyVote == -1)
        {
            voteText = tr("Abstaining.");
        }

        // If we've already voted, display a label with vote status above the vote button
        auto *voteLbl = new QLabel;
        if (!voteText.isEmpty()) {
            voteLbl->setText(voteText);
            boxLayout->addWidget(voteLbl, 0, Qt::AlignCenter);
            boxLayout->addSpacing(5);
        } else if (entry.fCanVote) {
            voteLbl->setText(tr("Did not vote."));
            boxLayout->addWidget(voteLbl, 0, Qt::AlignCenter);
            boxLayout->addSpacing(5);
        } else {
            voteLbl->setText(tr("Voting finished."));
            boxLayout->addWidget(voteLbl, 0, Qt::AlignCenter);
            boxLayout->addSpacing(5);
        }

        // Only show vote button if proposal voting is in progress
        if (entry.fCanVote) {
            auto *button = new QPushButton;
            button->setText(voteText.isEmpty() ? tr("Vote") : tr("Change"));
            button->setFixedHeight(40);
            button->setProperty("id", QString::fromStdString(entry.hash.GetHex()));
            boxLayout->addWidget(button, 0, Qt::AlignCenter);
            connect(button, SIGNAL(clicked()), this, SLOT(onVote()));
        }

        table->setCellWidget(i, PR_COLUMN_VOTE, widget);
        table->setItem(i, PR_COLUMN_VOTE, votedItem);
    }

    uint64_t nWeight = 0;
    if (pwalletMain)
        nWeight = pwalletMain->GetStakeWeight();
    bool fWeight = nWeight > 0;
    if (!fWeight) table->setColumnWidth(PR_COLUMN_VOTE, 0);
    else table->setColumnWidth(PR_COLUMN_VOTE, 150);
    table->setColumnHidden(PR_COLUMN_VOTE, !fWeight);
    table->setSortingEnabled(true);
}

void DaoPage::setData(QVector<ConsultationEntry> data)
{
    table->clearContents();
    table->setRowCount(data.count());
    table->setSortingEnabled(false);

    for (int i = 0; i < data.count(); ++i) {
        ConsultationEntry &entry = data[i];

        // HASH
        auto *hashItem = new QTableWidgetItem;
        hashItem->setData(Qt::DisplayRole, QString::fromStdString(entry.hash.GetHex()));
        table->setItem(i, C_COLUMN_HASH, hashItem);

        // COLOR
        auto *colorItem = new QTableWidgetItem;
        auto *indicatorBox = new QFrame;
        indicatorBox->setContentsMargins(QMargins());
        indicatorBox->setStyleSheet(QString("background-color: %1;").arg(entry.color));
        indicatorBox->setFixedWidth(3);
        table->setCellWidget(i, C_COLUMN_COLOR, indicatorBox);
        table->setItem(i, C_COLUMN_COLOR, colorItem);

        // TITLE
        auto *titleItem = new QTableWidgetItem;
        titleItem->setData(Qt::DisplayRole, entry.question);
        table->setItem(i, C_COLUMN_TITLE, titleItem);

        // ANSWERS
        QString answers;
        if(entry.fRange)
        {
            answers = QString(tr("Between %1 and %2")).arg(QString::number(entry.nMin), QString::number(entry.nMax));
        }
        else
        {
            QStringList s;
            for (auto& it: entry.answers)
            {
                s << it.answer;
            }
            s.sort();
            answers = s.join(", ");
        }

        auto *answersItem = new QTableWidgetItem;
        answersItem->setData(Qt::DisplayRole, answers);
        answersItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, C_COLUMN_ANSWERS, answersItem);

        // STATE
        auto *statusItem = new QTableWidgetItem;
        statusItem->setData(Qt::DisplayRole, entry.sState);
        table->setItem(i, C_COLUMN_STATUS, statusItem);

        // MY VOTES
        auto *myvotesItem = new QTableWidgetItem;
        myvotesItem->setData(Qt::DisplayRole, entry.myVotes.join(", "));
        myvotesItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, C_COLUMN_MY_VOTES, myvotesItem);

        //VOTE
        auto *votedItem = new QTableWidgetItem;
        auto *widget = new QWidget();
        widget->setContentsMargins(QMargins());
        widget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        auto *boxLayout = new QHBoxLayout;
        boxLayout->setContentsMargins(QMargins());
        boxLayout->setSpacing(0);
        widget->setLayout(boxLayout);

        // Only show vote button if proposal voting is in progress
        if (entry.fCanVote || (entry.fState == DAOFlags::NIL && entry.fCanSupport)) {
            auto *button = new QPushButton;
            if (entry.fState == DAOFlags::NIL && entry.fCanSupport)
                button->setText(entry.myVotes.size() == 0 ? tr("Support") : tr("Unsupport"));
            else if(entry.fCanVote)
                button->setText(entry.myVotes.size() == 0 ? tr("Vote") : tr("Change"));
            button->setFixedHeight(40);
            button->setProperty("id", QString::fromStdString(entry.hash.GetHex()));
            boxLayout->addWidget(button, 0, Qt::AlignCenter);
            connect(button, SIGNAL(clicked()), this, SLOT(onVote()));
        }

        table->setCellWidget(i, C_COLUMN_VOTE, widget);
        table->setItem(i, C_COLUMN_VOTE, votedItem);

    }

    uint64_t nWeight = 0;
    if (pwalletMain)
        nWeight = pwalletMain->GetStakeWeight();
    bool fWeight = nWeight > 0;
    if (!fWeight) {
        table->setColumnWidth(C_COLUMN_VOTE, 0);
        table->setColumnWidth(C_COLUMN_MY_VOTES, 0);
    }
    else
    {
        table->setColumnWidth(C_COLUMN_VOTE, 150);
        table->setColumnWidth(C_COLUMN_MY_VOTES, 150);

    }
    table->setColumnHidden(C_COLUMN_VOTE, !fWeight);
    table->setColumnHidden(C_COLUMN_MY_VOTES, !fWeight);
    table->setSortingEnabled(true);
}

void DaoPage::setData(QVector<DeploymentEntry> data)
{
    table->clearContents();
    table->setRowCount(data.count());
    table->setSortingEnabled(false);

    for (int i = 0; i < data.count(); ++i) {
        DeploymentEntry &entry = data[i];

        // COLOR
        auto *colorItem = new QTableWidgetItem;
        auto *indicatorBox = new QFrame;
        indicatorBox->setContentsMargins(QMargins());
        indicatorBox->setStyleSheet(QString("background-color: %1;").arg(entry.color));
        indicatorBox->setFixedWidth(3);
        table->setCellWidget(i, D_COLUMN_COLOR, indicatorBox);
        table->setItem(i, D_COLUMN_COLOR, colorItem);

        // TITLE
        auto *titleItem = new QTableWidgetItem;
        titleItem->setData(Qt::DisplayRole, entry.title);
        table->setItem(i, D_COLUMN_TITLE, titleItem);

        // STATE
        auto *statusItem = new QTableWidgetItem;
        statusItem->setData(Qt::DisplayRole, entry.status);
        statusItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, D_COLUMN_STATUS, statusItem);

        // MY VOTES
        auto *myvotesItem = new QTableWidgetItem;
        myvotesItem->setData(Qt::DisplayRole, entry.fMyVote ? tr("Yes") : tr("No"));
        myvotesItem->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
        table->setItem(i, D_COLUMN_MY_VOTES, myvotesItem);

        // VOTE
        auto *votedItem = new QTableWidgetItem;
        auto *widget = new QWidget();
        widget->setContentsMargins(QMargins());
        widget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        auto *boxLayout = new QHBoxLayout;
        boxLayout->setContentsMargins(QMargins());
        boxLayout->setSpacing(0);
        widget->setLayout(boxLayout);

        // Only show vote button if proposal voting is in progress
        if (entry.fCanVote) {
            auto *button = new QPushButton;
            button->setText(!entry.fDefaultVote ? tr("Vote %1").arg(!entry.fMyVote ? tr("Yes") : tr("No")) : tr("Change to %1").arg(!entry.fMyVote ? tr("Yes") : tr("No")));
            button->setFixedHeight(40);
            button->setProperty("id", entry.bit);
            button->setProperty("vote", !entry.fMyVote);
            boxLayout->addWidget(button, 0, Qt::AlignCenter);
            connect(button, SIGNAL(clicked()), this, SLOT(onVote()));
        }

        table->setCellWidget(i, D_COLUMN_VOTE, widget);
        table->setItem(i, D_COLUMN_VOTE, votedItem);
    }

    uint64_t nWeight = 0;
    if (pwalletMain)
        nWeight = pwalletMain->GetStakeWeight();
    bool fWeight = nWeight > 0;
    if (!fWeight) {
        table->setColumnWidth(D_COLUMN_VOTE, 0);
        table->setColumnWidth(D_COLUMN_MY_VOTES, 0);
    }
    else
    {
        table->setColumnWidth(D_COLUMN_VOTE, 150);
        table->setColumnWidth(D_COLUMN_MY_VOTES, 150);

    }
    table->setColumnHidden(D_COLUMN_VOTE, !fWeight);
    table->setColumnHidden(D_COLUMN_MY_VOTES, !fWeight);
    table->setSortingEnabled(true);
}

void DaoPage::onVote() {
    LOCK(cs_main);

    QVariant idV = sender()->property("id");
    QVariant voteV = sender()->property("vote");
    CStateViewCache coins(pcoinsTip);

    if (idV.isValid() && voteV.isValid())
    {
        VoteVersionBit(idV.toInt(), voteV.toBool());
    }
    else if (idV.isValid())
    {
        CProposal proposal;
        CPaymentRequest prequest;
        CConsultation consultation;
        CConsultationAnswer answer;

        uint256 hash = uint256S(idV.toString().toStdString());

        if (coins.GetProposal(hash, proposal))
        {
            CommunityFundDisplayDetailed dlg(this, proposal);
            dlg.exec();
            refresh(true, true);
        }
        else if (coins.GetPaymentRequest(hash, prequest))
        {
            CommunityFundDisplayPaymentRequestDetailed dlg(this, prequest);
            dlg.exec();
            refresh(true, true);
        }
        else if (coins.GetConsultation(hash, consultation))
        {
            if (consultation.fState == DAOFlags::NIL)
            {
                bool duplicate;
                if (mapSupported.count(consultation.hash))
                    RemoveSupport(consultation.hash.ToString());
                else
                    Support(consultation.hash, duplicate);
                refresh(true, true);
            }
            else
            {
                DaoConsultationVote dlg(this, consultation);
                dlg.exec();
                refresh(true, true);
            }
        }
        else if (coins.GetConsultationAnswer(hash, answer))
        {
            if (answer.fState == DAOFlags::NIL)
            {
                bool duplicate;
                if (mapSupported.count(answer.hash))
                    RemoveSupport(answer.hash.ToString());
                else
                    Support(answer.hash, duplicate);
                refresh(true, true);
            }
        }
    }
}

void DaoPage::onDetails() {
    if (!contextHash.isEmpty())
    {
        QString type;
        if (nCurrentView == VIEW_PROPOSALS)
            type = "proposal";
        else if (nCurrentView == VIEW_PAYMENT_REQUESTS)
            type = "payment-request";
        else if (nCurrentView == VIEW_CONSULTATIONS)
            type = "consultation";
        QString link = "https://www.navexplorer.com/community-fund/" + type + "/" + contextHash;
        QDesktopServices::openUrl(QUrl(link));
    }
}

void DaoPage::viewProposals() {
    setView(VIEW_PROPOSALS);
    if (filterHash.isEmpty())
    {
        if (nBadgeProposals > 0)
        {
            onFilter(FILTER_NOT_VOTED);
            onFilter2(FILTER2_IN_PROGRESS);
        }
        else
        {
            onFilter(FILTER_ALL);
            onFilter2(FILTER2_ALL);
        }
    }
    proposalsBtn->setStyleSheet("QPushButton { background-color: #DBE0E8; }");
    paymentRequestsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    consultationsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    deploymentsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
}

void DaoPage::viewPaymentRequests() {
    setView(VIEW_PAYMENT_REQUESTS);
    if (filterHash.isEmpty())
    {
        if (nBadgePaymentRequests > 0)
        {
            onFilter(FILTER_NOT_VOTED);
            onFilter2(FILTER2_IN_PROGRESS);
        }
        else
        {
            onFilter(FILTER_ALL);
            onFilter2(FILTER2_ALL);
        }
    }
    proposalsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    paymentRequestsBtn->setStyleSheet("QPushButton { background-color: #DBE0E8; }");
    consultationsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    deploymentsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
}

void DaoPage::viewConsultations() {
    setView(VIEW_CONSULTATIONS);
    if (nBadgeConsultations > 0)
    {
        onFilter(FILTER_NOT_VOTED);
        onFilter2(FILTER2_IN_PROGRESS);
    }
    else
    {
        onFilter(FILTER_ALL);
        onFilter2(FILTER2_ALL);
    }
    proposalsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    paymentRequestsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    consultationsBtn->setStyleSheet("QPushButton { background-color: #DBE0E8; }");
    deploymentsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
}

void DaoPage::viewDeployments() {
    setView(VIEW_DEPLOYMENTS);
    if (nBadgeDeployments > 0)
    {
        onFilter(FILTER_NOT_VOTED);
        onFilter2(FILTER2_IN_PROGRESS);
    }
    else
    {
        onFilter(FILTER_ALL);
        onFilter2(FILTER2_ALL);
    }
    proposalsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    paymentRequestsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    consultationsBtn->setStyleSheet("QPushButton { background-color: #EDF0F3; }");
    deploymentsBtn->setStyleSheet("QPushButton { background-color: #DBE0E8; }");
}

void DaoPage::backToFilter() {
    filterHash = "";
    if (nCurrentView == VIEW_PROPOSALS)
        viewPaymentRequests();
    else if (nCurrentView == VIEW_PAYMENT_REQUESTS)
        viewProposals();
}

void DaoPage::onFilter(int index) {
    nFilter = index;
    filterCmb->setCurrentIndex(index);
    refresh(true);
}

void DaoPage::onFilter2(int index) {
    nFilter2 = index;
    filter2Cmb->setCurrentIndex(index);
    refresh(true);
}

void DaoPage::onExclude(bool fChecked) {
    fExclude = fChecked;
    refresh(true);
}

void DaoPage::onCreate() {
    if (nCurrentView == VIEW_PROPOSALS)
    {
        CommunityFundCreateProposalDialog dlg(this);
        dlg.setModel(walletModel);
        dlg.exec();
        refresh(true);
    }
    else if (nCurrentView == VIEW_PAYMENT_REQUESTS)
    {
        CommunityFundCreatePaymentRequestDialog dlg(this);
        dlg.setModel(walletModel);
        dlg.exec();
        refresh(true);
    }
    else if (nCurrentView == VIEW_CONSULTATIONS)
    {
        DaoConsultationCreate dlg(this);
        dlg.setModel(walletModel);
        dlg.exec();
        refresh(true);
    }
}

void DaoPage::onProposeAnswer() {
    LOCK(cs_main);

    CStateViewCache coins(pcoinsTip);
    CConsultation consultation;

    if (coins.GetConsultation(uint256S(contextHash.toStdString()), consultation))
    {
        if (consultation.CanHaveNewAnswers())
        {
            DaoProposeAnswer dlg(this, consultation, [](QString s)->bool{return !s.isEmpty();});
            dlg.setModel(walletModel);
            dlg.exec();
            refresh(true);
        }
    }
}

void DaoPage::onSupportAnswer() {
    LOCK(cs_main);

    CStateViewCache coins(pcoinsTip);
    CConsultation consultation;

    if (coins.GetConsultation(uint256S(contextHash.toStdString()), consultation))
    {
        if (!consultation.IsRange())
        {
            DaoSupport dlg(this, consultation);
            dlg.setModel(walletModel);
            dlg.exec();
            refresh(true);
        }
    }
}

void DaoPage::showContextMenu(const QPoint& pt) {
    if (nView == VIEW_DEPLOYMENTS)
        return;

    auto *item = table->itemAt(pt);
    contextHash = "";
    if (!item) {
        contextItem = nullptr;
        return;
    }
    contextItem = item;
    auto *hashItem = table->item(contextItem->row(), P_COLUMN_HASH);
    if (hashItem) {
        LOCK(cs_main);

        CConsultation consultation;
        CProposal proposal;
        CPaymentRequest prequest;

        contextHash = hashItem->data(Qt::DisplayRole).toString();

        if (pcoinsTip->GetConsultation(uint256S(contextHash.toStdString()), consultation))
        {
            if (consultation.CanHaveNewAnswers())
            {
                contextMenu->addAction(supportAnswer);
                contextMenu->addAction(proposeAnswer);
            }
            else
            {
                contextMenu->removeAction(supportAnswer);
                contextMenu->removeAction(proposeAnswer);
            }
        }
        else if (pcoinsTip->GetProposal(uint256S(contextHash.toStdString()), proposal))
        {
            contextMenu->addAction(seePaymentRequestsAction);
            contextMenu->removeAction(seeProposalAction);

        }
        else if (pcoinsTip->GetPaymentRequest(uint256S(contextHash.toStdString()), prequest))
        {
            contextMenu->addAction(seeProposalAction);
            contextMenu->removeAction(seePaymentRequestsAction);
        }
    }
    contextMenu->exec(QCursor::pos());
}

void DaoPage::onCopyHash() {
    if (contextHash.isEmpty())
        return;
    QApplication::clipboard()->setText(contextHash, QClipboard::Clipboard);
}

void DaoPage::onSeePaymentRequests() {
    if (contextHash.isEmpty())
        return;

    filterHash = contextHash;

    viewPaymentRequests();
}

void DaoPage::onSeeProposal() {
    if (contextHash.isEmpty())
        return;

    LOCK(cs_main);

    CPaymentRequest prequest;

    if (pcoinsTip->GetPaymentRequest(uint256S(contextHash.toStdString()), prequest))
    {
        filterHash = QString::fromStdString(prequest.proposalhash.ToString());
    }

    viewProposals();
}
