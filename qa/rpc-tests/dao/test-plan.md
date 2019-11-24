# NavCoin CFund Test Plan

This file is a map of the tests scenarios which will be covered to ensure the CFund is fully tested. This is currently a work in progress, so if you notice there are missing scenarios please add them to this file following the same format/template. If you want to author a test based on a scenario here then please go ahead and claim it. Just make sure you claim and check off any scenarios which you write so others are aware of what is still todo.

## Scenarios

### Scenario 001

| ID          |  CFUND_003 |
| ----------- | -----: |
| Reporter    | @proletesseract |
| Author      | @proletesseract |
| Satus       | Done |
| Description | It should create a proposal and let it expire |
| File        | `./001-proposal-expired.py`

1. **Given** I have created a proposal
2. **When** The proposal is not voted on
3. **Then** the proposal should expire 

#### Test Steps
- Activate CFund
- Donate to the CFund
- Create a proposal
- Do not cast any votes
- End the full round of voting cycles
- Check proposal expired

### Scenario 002

| ID          |  CFUND_002 |
| ----------- | -----: |
| Reporter    | @proletesseract |
| Author      | @proletesseract |
| Satus       | Done |
| Description | It should create a proposal and the network should reject the proposal |
| File        | `./002-proposal-rejected.py`

1. **Given** I have created a proposal
2. **When** The proposal receives the consensus of `no` votes
3. **Then** the proposal should be rejected 

#### Test Steps
- Activate CFund
- Donate to the CFund
- Create a proposal
- Votes `no` on the proposal
- End the full round of voting cycles
- Check proposal is rejected

### Scenario 003

| ID          |  CFUND_003 |
| ----------- | -----: |
| Reporter    | @proletesseract |
| Author      | @proletesseract |
| Satus       | Done |
| Description | It should create a proposal and the network should accept the proposal |
| File        | `./003-proposal-accepted.py`

1. **Given** I have created a proposal
2. **When** The proposal receives the consensus of `yes` votes
3. **Then** the proposal should be accepted 

#### Test Steps
- Activate CFund
- Donate to the CFund
- Create a proposal
- Votes `yes` on the proposal
- End the full round of voting cycles
- Check proposal is rejected

### Scenario 004

| ID          |  CFUND_004 |
| ----------- | -----: |
| Reporter    | @proletesseract |
| Author      | @proletesseract |
| Satus       | Done |
| Description | It should fail to create a payment request for an expired proposal |
| File        | `./004-proposal-expired-preq.py`

1. **Given** I have an expired proposal
2. **When** A payment request is made
3. **Then** the payment request should be rejected

#### Test Steps
- Activate CFund
- Donate to the CFund
- Create a proposal
- Do not cast any votes
- End the full round of voting cycles
- Check payment request is rejected

### Scenario 005

| ID          |  CFUND_005 |
| ----------- | -----: |
| Reporter    | @proletesseract |
| Author      | @proletesseract |
| Satus       | Done |
| Description | It should fail to create a payment request for a rejected proposal |
| File        | `./005-proposal-expired-preq.py`

1. **Given** I have a rejected proposal
2. **When** A payment request is made
3. **Then** the payment request should be rejected

#### Test Steps
- Activate CFund
- Donate to the CFund
- Create a proposal
- Votes `no` on the proposal
- End the full round of voting cycles
- Check payment request is rejected