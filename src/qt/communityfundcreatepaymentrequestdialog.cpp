#include "communityfundcreatepaymentrequestdialog.h"
#include "ui_communityfundcreatepaymentrequestdialog.h"

#include "sendcommunityfunddialog.h"
#include "consensus/cfund.h"
#include "main.h"
#include "main.cpp"
#include "guiconstants.h"
#include "skinize.h"
#include <QMessageBox>
#include <iostream>

std::string random_string_owo( size_t length )
{
    auto randchar = []() -> char
    {
        const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[ rand() % max_index ];
    };
    std::string str(length,0);
    std::generate_n( str.begin(), length, randchar );
    return str;
}

CommunityFundCreatePaymentRequestDialog::CommunityFundCreatePaymentRequestDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CommunityFundCreatePaymentRequestDialog)
{
    ui->setupUi(this);

    //connect
    connect(ui->pushButtonClose, SIGNAL(clicked()), this, SLOT(reject()));
    connect(ui->pushButtonSubmitPaymentRequest, SIGNAL(clicked()), SLOT(click_pushButtonSubmitPaymentRequest()));
}

bool CommunityFundCreatePaymentRequestDialog::validate()
{
    bool isValid = true;

    //proposal hash;
    if(!isActiveProposal(uint256S(ui->lineEditProposalHash->text().toStdString())))
    {
        isValid = false;
        ui->lineEditProposalHash->setValid(false);
    }
    //amount
    if(!ui->lineEditRequestedAmount->validate())
    {
        isValid = false;
        ui->lineEditRequestedAmount->setValid(false);
    }

    //desc
    size_t desc_size = ui->plainTextEditDescription->toPlainText().toStdString().length();
    if(desc_size >= 1024 || desc_size == 0)
    {
        isValid = false;
        ui->plainTextEditDescription->setValid(false);
    }
    else
    {
        ui->plainTextEditDescription->setValid(true);
    }

    return isValid;
}

bool CommunityFundCreatePaymentRequestDialog::click_pushButtonSubmitPaymentRequest()
{

    if(this->validate())
    {
        LOCK2(cs_main, pwalletMain->cs_wallet);

        // Get Proposal
        CFund::CProposal proposal;
        if(!CFund::FindProposal(ui->lineEditProposalHash->text().toStdString(), proposal)) {
            QMessageBox msgBox(this);
            std::string str = "Proposal could not be found with that hash\n";
            msgBox.setText(tr(str.c_str()));
            msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Proposal not found");
            msgBox.exec();
            return false;
        }
        if(proposal.fState != CFund::ACCEPTED) {
            QMessageBox msgBox(this);
            std::string str = "Proposals need to have been accepted to create a Payment Request for them\n";
            msgBox.setText(tr(str.c_str()));
            msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Proposal not accepted");
            msgBox.exec();
            return false;
        }

        // Get Address
        CNavCoinAddress address(proposal.Address);
        if(!address.IsValid()) {
            QMessageBox msgBox(this);
            std::string str = "The address of the Proposal is not valid\n";
            msgBox.setText(tr(str.c_str()));
            msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Address not valid");
            msgBox.exec();
            return false;
        }

        // Get KeyID
        CKeyID keyID;
        if (!address.GetKeyID(keyID)) {
            QMessageBox msgBox(this);
            std::string str = "The address does not refer to a key\n";
            msgBox.setText(tr(str.c_str()));
            msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Address not valid");
            msgBox.exec();
            return false;
        }

        //EnsureWalletIsUnlocked(); TODO Replace this

        // Get Key
        CKey key;
        if (!pwalletMain->GetKey(keyID, key)) {
            QMessageBox msgBox(this);
            std::string str = "You are not the owner of the Proposal\n";
            msgBox.setText(tr(str.c_str()));
            msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Not the owner");
            msgBox.exec();
            return false;
        }

        // Get fields from form
        CAmount nReqAmount = ui->lineEditRequestedAmount->value();
        std::string id = ui->plainTextEditDescription->toPlainText().toStdString();
        std::string sRandom = random_string_owo(16); // Check this implementation

        // Construct Secret
        std::string Secret = sRandom + "I kindly ask to withdraw " +
                std::to_string(nReqAmount) + "NAV from the proposal " +
                proposal.hash.ToString() + ". Payment request id: " + id;

        CHashWriter ss(SER_GETHASH, 0);
        ss << strMessageMagic;
        ss << Secret;

        // Attempt to sign
        vector<unsigned char> vchSig;
        if (!key.SignCompact(ss.GetHash(), vchSig)) {
            QMessageBox msgBox(this);
            std::string str = "Failed to sign\n";
            msgBox.setText(tr(str.c_str()));
            msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Sign Failed");
            msgBox.exec();
            return false;
        }

        // Create Signature
        std::string Signature = EncodeBase64(&vchSig[0], vchSig.size());

        // Validate requested amount
        if (nReqAmount <= 0 || nReqAmount > proposal.GetAvailable(true)) {
            QMessageBox msgBox(this);
            std::string str = "Cannot create a Payment Request for the requested amount\n";
            msgBox.setText(tr(str.c_str()));
            msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Invalid Amount");
            msgBox.exec();
            return false;
        }

        // Create wtx
        CWalletTx wtx;
        bool fSubtractFeeFromAmount = false;

        UniValue strDZeel(UniValue::VOBJ);

        strDZeel.push_back(Pair("h",ui->lineEditProposalHash->text().toStdString()));
        strDZeel.push_back(Pair("n",nReqAmount));
        strDZeel.push_back(Pair("s",Signature));
        strDZeel.push_back(Pair("r",sRandom));
        strDZeel.push_back(Pair("i",id));
        strDZeel.push_back(Pair("v",IsReducedCFundQuorumEnabled(pindexBestHeader, Params().GetConsensus()) ? CFund::CPaymentRequest::CURRENT_VERSION : 2));

        wtx.strDZeel = strDZeel.write();
        wtx.nCustomVersion = CTransaction::PAYMENT_REQUEST_VERSION;

        // Validate wtx
        if(wtx.strDZeel.length() > 1024) {
            QMessageBox msgBox(this);
            std::string str = "String too long\n";
            msgBox.setText(tr(str.c_str()));
            msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Invalid String");
            msgBox.exec();
            return false;
        }

        // TODO Implement the SendMoney() part
        //SendMoney(address.Get(), 10000, fSubtractFeeFromAmount, wtx, "", true);

        bool donate = true;
        CAmount curBalance = pwalletMain->GetBalance();

        if (nReqAmount <= 0)
            return false;

        if (nReqAmount > curBalance)
            return false;

        // Parse NavCoin address (currently crashes wallet)
        CScript scriptPubKey = GetScriptForDestination(address.Get());

        if(donate)
          CFund::SetScriptForCommunityFundContribution(scriptPubKey);

        // Create and send the transaction
        CReserveKey reservekey(pwalletMain);
        CAmount nFeeRequired;
        std::string strError;
        vector<CRecipient> vecSend;
        int nChangePosRet = -1;
        CRecipient recipient = {scriptPubKey, nReqAmount, fSubtractFeeFromAmount, ""};
        vecSend.push_back(recipient);

        //create confirmation dialog
        {
        CFund::CPaymentRequest* preq = new CFund::CPaymentRequest();
        preq->nAmount = proposal.nAmount;
        preq->fState = proposal.fState;
        preq->strDZeel = proposal.strDZeel;
        SendCommunityFundDialog dlg(this, preq, 10);
        if(dlg.exec()== QDialog::Rejected)
            return false;
        }

        if (!pwalletMain->CreateTransaction(vecSend, wtx, reservekey, nFeeRequired, nChangePosRet, strError, NULL, true, strDZeel.get_str())) {
            if (!fSubtractFeeFromAmount && nReqAmount + nFeeRequired > pwalletMain->GetBalance());
        }
        if (!pwalletMain->CommitTransaction(wtx, reservekey));
        return true;

    }
    else
    {

        QMessageBox msgBox(this);
        std::string str = "Please enter a valid:\n";
        if(!isActiveProposal(uint256S(ui->lineEditProposalHash->text().toStdString())))
            str += "- Proposal Hash\n";
        if(!ui->lineEditRequestedAmount->validate())
            str += "- Requested Amount\n";
        if(ui->plainTextEditDescription->toPlainText() == QString("") || ui->plainTextEditDescription->toPlainText().size() <= 0)
            str += "- Description\n";

        msgBox.setText(tr(str.c_str()));
        msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
        return false;
    }
}
bool CommunityFundCreatePaymentRequestDialog::isActiveProposal(uint256 hash)
{
    std::vector<CFund::CProposal> vec;
    if(pblocktree->GetProposalIndex(vec))
    {
        if(std::find_if(vec.begin(), vec.end(), [&hash](CFund::CProposal& obj) {return obj.hash == hash;}) == vec.end())
        {
            return false;
        }
    }

    return true;
}

CommunityFundCreatePaymentRequestDialog::~CommunityFundCreatePaymentRequestDialog()
{
    delete ui;
}


