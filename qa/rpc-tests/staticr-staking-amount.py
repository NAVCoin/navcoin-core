#!/usr/bin/env python3
# Copyright (c) 2018 The Navcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import NavCoinTestFramework
from test_framework.staticr_util import *

import time

class StaticRAmountTest(NavCoinTestFramework):
    """Tests the staking amount after softfork activation."""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self, split=False):
        self.nodes = self.setup_nodes()
        self.is_network_split = split

    def run_test(self):
        activate_staticr(self.nodes[0])

        print("blockcount: " + str(self.nodes[0].getblockcount()))
        blockcount = self.nodes[0].getblockcount())
        wallet_info = self.nodes[0].getwalletinfo()
        print(wallet_info)

        while self.nodes[0].getblockcount() == blockcount:
            print("No new block, going back to sleep...")
            time.sleep(5)

        print("Sometime later...")
        assert(blockcount != self.nodes[0].getblockcount())

        wallet_info2 = self.nodes[0].getwalletinfo()
        print(wallet_info2)

        balance_diff = wallet_info['balance'] - wallet_info2['balance']

        assert(wallet_info2['immature_balance'] == wallet_info['immature_balance'] + balance_diff + 2)

if __name__ == '__main__':
    StaticRAmountTest().main()
