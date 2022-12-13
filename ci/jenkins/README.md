# Local Jenkins in Docker Compose Setup

A quick, easy-to-setup Jenkins environment, intended to be run on the
developer's local computer to monitor progress; i.e. view test results,
code coverage, etc, is presented here.

The Jenkins setup is based on a primary controller Jenkins container named
_jenkins_ and a permanent worker agent named _agent_.


## Host-side Setup

Make a *jenkins_home* directory is created in your home directory before
proceeding:

    mkdir ~/jenkins_home

Generate the needed environment variables and SSH key pair for Jenkins agent
communication by running the supplied script:

    cd dev-can-linux/ci/jenkins/
    ./generate-keys-envs.sh

This will create a *.env*, *.jenkins_agent* and *.jenkins_agent.pub* files you
will require in the following steps.


## Jenkins Start-up

Now start up the Jenkins containers using Docker Compose:

    docker-compose up -d

Open your browser on your local host to _http://localhost:8080/_.

You will see the initial Jenkins page presenting the _Unlock Jenkins_ page; you
will need to enter the hex code, which you can attain from the logs by running:

    docker logs jenkins

Here is an example output with _some hex code here_ marker:

    *************************************************************
    *************************************************************
    *************************************************************
    
    Jenkins initial setup is required. An admin user has been created and a
    password generated.
    Please use the following password to proceed to installation:
    
    <some hex code here>
    
    This may also be found at: /var/jenkins_home/secrets/initialAdminPassword
    
    *************************************************************
    *************************************************************
    *************************************************************

You can also just check from your local *~/jenkins_home* path:

    cat ~/jenkins_home/secrets/initialAdminPassword

If needed you can login to the _jenkins_ container as follows:

    docker exec --interactive --tty --user jenkins \
        --workdir /var/jenkins_home/ jenkins /bin/bash --login

Similarly to login to the _agent_ containers:

    docker exec --interactive --tty --user jenkins \
        --workdir /home/jenkins agent /bin/bash --login

If needed you can also login as root to both of these containers; this could
come handy when troubleshooting issues:

    docker exec --interactive --tty --user root \
        --workdir /root/ <container-name> /bin/bash --login

Where <container-name> can be _jenkins_ or _agent_.


## Jenkins Plugins

On startup choose the option to install recommended plugins.

In addition to those, install plugins:

- _Last Changes_


## Jenkins Manage Credentials

Click on the Manage Jenkins menu on the side panel.

Then go to Manage Credentials.

Click on the _(global)_ text under _Domains_.

_Global credentials (unrestricted)_ page will open, click _Add Credentials_.

Set these options on this screen.

- Select SSH Username with private key

- Scope as System (Jenkins and nodes only)

- Pick an ID; e.g. jenkins

- Add a description; e.g. "jenkins agent connection"

- Enter _jenkins_ for a username

Check the _Private Key_ checkbox item and copy/paste the text from the
*.jenkins_agent* private key.

Leave the _Passphrase_ empty.


## Jenkins Manage Nodes and Clouds

Again click on the _Manage Jenkins_, and select _Manage Nodes and Clouds_
instead of credentials.

Click _+ New Node_, pick _Node name_ as _jenkins_agent_ and check
_Permanent Agent_ and click _Create_.

Leave _Number of executors_ as _1_.

Set _Remote root directory_ as _/home/jenkins/agent_.

For _Usage_ select _Use this node as much as possible_.

For _Launch method_ select _Launch agents via SSH_.

Enter _agent_ as _Host_ and select the credentials we made in the previous
section.

For the _Host Key Verification Strategy_, select _Non verifying Verification
Strategy_.

Now click _Advanced..._ to reveal more options.

For _Port_ put _22_.

For _JavaPath_ put _/opt/java/openjdk/bin/java_.

Set _Connection Timeout in Seconds_ to 60, _Maximum Number of Retries_ to 10,
and _Seconds To Wait Between Retries_ to 15.


## Pipeline

We intend this Jenkins setup to be a local and convenient setup that can test
your local changes without you needing to commit them. For this reason we simply
mount the current host Git repository to the Jenkins container
_/opt/dev-can-linux_ path. The build process will make disposable copies
internally to prevent modifications.

Configure a Pipeline named _DEV-CAN-LINUX_ with _Definition_ as _Pipeline script
from SCM_ and _SCM_ type as _Git_, with the following settings:

- _Repository URL_: /opt/dev-can-linux
- _Credentials_: empty
- _Branches to build_: \*/main

Then specify the _Script Path_ as _ci/jenkins/Jenkinsfile_.

You should be able to click on _DEV-CAN-LINUX_ from the _Dashboard_ and then
click _Build Now_ to start the pipeline.


## Troubleshooting

Note Jenkins doesn't seem to allow connection when using the default
_ssh-keygen_ RSA key, you either have to specify for example '-b 4096' or just
use the recommended ed25519, which is what we do.

If the agent connection isn't working here are some checks you can do.

Login to the Jenkins _agent_ container and verify the _authorized_keys_ have
been installed by the environment variable _JENKINS_AGENT_SSH_PUBKEY_:

    $ docker exec --interactive --tty --user jenkins \
        --workdir /home/jenkins agent /bin/bash --login

    jenkins@agent:~$ cat .ssh/authorized_keys 
    ssh-ed25519 *<.jenkins_agent.pub file contents>*

Login to the Jenkins controller _jenkins_ container and verifty SSH to the
_agent_ using the private key _.jenkins_agent_ works:

    $ docker exec --interactive --tty --user root \
        --workdir /root/ jenkins /bin/bash --login

    root@jenkins:~# ssh -i ./dev-can-linux/ci/jenkins/.jenkins_agent jenkins@agent

