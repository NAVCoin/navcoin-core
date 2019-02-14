#include "communityfundcreateproposaldialog.h"
#include "ui_communityfundcreateproposaldialog.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "sync.h"
#include "wallet/wallet.h"
#include "base58.h"
#include "main.h"
#include "wallet/rpcwallet.h"
#include "wallet/rpcwallet.cpp"
#include <string>

CommunityFundCreateProposalDialog::CommunityFundCreateProposalDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CommunityFundCreateProposalDialog)
{
    ui->setupUi(this);
    GUIUtil::setupAddressWidget(ui->lineEditNavcoinAddress, this);

    ui->spinBoxDays->setRange(0, 999999999);
    ui->spinBoxHours->setRange(0, 59);
    ui->spinBoxMinutes->setRange(0, 59);

    //connect
    connect(ui->pushButtonClose, SIGNAL(clicked()), this, SLOT(reject()));
    connect(ui->pushButtonCreateProposal, SIGNAL(clicked()), this, SLOT(on_click_pushButtonCreateProposal()));
}

// validate input fields
bool CommunityFundCreateProposalDialog::validate()
{
    bool isValid = true;
    if(!ui->lineEditNavcoinAddress->isValid())
    {
        ui->lineEditNavcoinAddress->setValid(false);
        isValid = false;
    }
    if(!ui->lineEditRequestedAmount->validate())
    {
        ui->lineEditRequestedAmount->setValid(false);
        isValid = false;
    }
    if(ui->plainTextEditDescription->toPlainText() == QString(""))
    {
        ui->plainTextEditDescription->setStyleSheet(STYLE_INVALID);
        isValid = false;
    } else {
        ui->plainTextEditDescription->setStyleSheet(styleSheet());
    }

    return isValid;
}

//Q_SLOTS

bool CommunityFundCreateProposalDialog::on_click_pushButtonCreateProposal()
{
    if(this->validate())
    {
        LOCK2(cs_main, pwalletMain->cs_wallet);

        CNavCoinAddress address("NQFqqMUD55ZV3PJEJZtaKCsQmjLT6JkjvJ"); // Dummy address
        std::cout << "created fake addr\n";
        CWalletTx wtx;
        bool fSubtractFeeFromAmount = false;

        std::string Address = ui->lineEditNavcoinAddress->text().toStdString().c_str();

        CNavCoinAddress destaddress(Address);
        if (!destaddress.IsValid())
          return false;

        CAmount nReqAmount = ui->lineEditRequestedAmount->value();
        int64_t nDeadline = ui->spinBoxDays->value()*24*60*60 + ui->spinBoxHours->value()*60*60 + ui->spinBoxMinutes->value()*60;

        if(nDeadline <= 0)
            return false;

        string sDesc = ui->plainTextEditDescription->toPlainText().toStdString();

        UniValue strDZeel(UniValue::VOBJ);

        strDZeel.push_back(Pair("n",nReqAmount));
        strDZeel.push_back(Pair("a",Address));
        strDZeel.push_back(Pair("d",nDeadline));
        strDZeel.push_back(Pair("s",sDesc));
        strDZeel.push_back(Pair("v",IsReducedCFundQuorumEnabled(pindexBestHeader, Params().GetConsensus()) ? CFund::CProposal::CURRENT_VERSION : 2));

        wtx.strDZeel = strDZeel.write();
        wtx.nCustomVersion = CTransaction::PROPOSAL_VERSION;

        if(wtx.strDZeel.length() > 1024)
            return false;

        EnsureWalletIsUnlocked();

        bool donate = true;
        CAmount curBalance = pwalletMain->GetBalance();

        if (nReqAmount <= 0)
            return false;

        if (nReqAmount > curBalance)
            return false;

        // Parse NavCoin address (currently crashes wallet)
        CScript scriptPubKey = GetScriptForDestination(address.Get());

        std::cout << "0";
        if(donate)
          CFund::SetScriptForCommunityFundContribution(scriptPubKey);
       std::cout << "1";
        // Create and send the transaction
        CReserveKey reservekey(pwalletMain);
        CAmount nFeeRequired;
        std::string strError;
        vector<CRecipient> vecSend;
        int nChangePosRet = -1;
        CRecipient recipient = {scriptPubKey, nReqAmount, fSubtractFeeFromAmount, ""};
        vecSend.push_back(recipient);
        std::cout << "2";
        if (!pwalletMain->CreateTransaction(vecSend, wtx, reservekey, nFeeRequired, nChangePosRet, strError, NULL, true, strDZeel.get_str())) {
            std::cout << "3";
            if (!fSubtractFeeFromAmount && nReqAmount + nFeeRequired > pwalletMain->GetBalance());
            std::cout << "4";
        }
        if (!pwalletMain->CommitTransaction(wtx, reservekey));
        std::cout << "5";

        UniValue ret(UniValue::VOBJ);

        ret.push_back(Pair("hash",wtx.GetHash().GetHex()));
        ret.push_back(Pair("strDZeel",wtx.strDZeel));

        std::cout << "all valid\n";
        return true;
    }
    return true;
}

CommunityFundCreateProposalDialog::~CommunityFundCreateProposalDialog()
{
    delete ui;
}
