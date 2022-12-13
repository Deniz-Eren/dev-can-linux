# DEV-CAN-LINUX development environment

A containerized QNX development environment setup is presented here based on
Ubuntu host operating system.

The Dockerfile configured in this section is also used by the local Jenkins
setup described in [ci/jenkins/](../ci/jenkins).

We have chosen _Podman_ as our containerization tool for it's convenient
handling of rootless installations, which is perfect for development
environments.


## Step 1

Make a copy of the _INIT_ template _qnx.Dockerfile_ provided:

    cd dev-can-linux/dev
    cp INIT.qnx.Dockerfile qnx.Dockerfile

## Step 2

Start the development environment with default Ubuntu base image:

    cd dev-can-linux/dev
    podman-compose up -d

## Step 3

Login to the development environment container and install your personal/private
licensed _QNX Software Center_ program:

    podman exec --interactive --tty --user root \
        --workdir /root/ qnx710-dev /bin/bash --login

We have tested this process with:

    QNX Software Center 2.0 Build 202209011607 - Linux Hosts

After downloading the installer file _qnx-setup-2.0-202209011607-linux.run_ from
your host machine, it will appear in your container in the following path (since
the _userhome_ mount in the development container is your home directory. To run
the installer simply:

    ./userhome/Downloads/qnx-setup-2.0-202209011607-linux.run

It will start as normal, but keep in mind this is installing inside the
development container:

    [press q to scroll to the bottom of this agreement]
    DEVELOPMENT LICENSE AGREEMENT
    Please type y to accept, n otherwise:

Accept all default installation paths:

    Specify installation path (default: /root/qnx):

Once started, the pop with _Log in to myQNX <@qnx-dev>_ will appear. Here login
and install your QNX 7.1 system & Momentics.

When prompted allow the QNX 7.1 system to install to the default location of
/root/qnx710 within the development container. The
[setup profile script](setup-profile.sh) assumes this path.


## Step 4

Once your QNX 7.1 system is installed and ready, Stop the _dev_ podman-compose
container instance:

    cd dev-can-linux/dev
    podman-compose stop

IMPORTANT! be sure NOT to use _down_ with podman-compose, just use _stop_ as
shown above. Using _down_ will delete the created container named _qnx710-dev_.

Next commit the container _qnx710-dev_ to your local Podman image registry:

    podman commit qnx710-dev <username>/dev-qnx710:1.0.0

Where <username> is either your repository user-name if you intend use a secure
remote private and personal repository as per _Step 6_ below, or just something
else unique you chose to use.

## Step 5

Our recommended container manager for our development environment is Podman,
however for [CI Jenkins pipelines](../ci/jenkins) we find it convenient to use
Docker. For this reason we need to clone our dev image to the local Docker
registry also as follows.

Save the created Podman container _qnx710-dev_ to a tar file:

    podman save localhost/<username>/dev-qnx710:1.0.0 | gzip > qnx710.tar.gz

Now import the saved image tar file to docker and retag without the _localhost_
prefix:

    docker load < qnx710.tar.gz
    docker tag localhost/<username>/dev-qnx710:1.0.0 <username>/dev-qnx710:1.0.0

You can now delete the temporary tar file.

## Step 6 (Optional)

If you chose to backup your image to a secure remote private and personal
repository, then these remaining steps apply. Ensure this is your own personal
repository and nobody else can access it, since this image now contains your
personal development environment and installed software. Otherwise you can skip
this step and go to the next step.

Login to your remote repository and push your image:

    docker login -u <username>
    docker push <username>/dev-qnx710:1.0.0

Be sure to clean up all the temporary tags and images created above during this
process for both your docker and podman local image repository. You can check
your local repos with command:

    docker image list

and

    podman image list

## Step 7

Edit your qnx.Dockerfile and change the FROM repo to be either your local
committed image name or your private remote repo.

For local committed image:

    #FROM ubuntu:22.04
    FROM localhost/<username>/dev-qnx710:1.0.0

For remote private repo:

    #FROM ubuntu:22.04
    FROM <repository-url>/<username>/dev-qnx710

Now you will have a disposable container setup where you can perform _down_ and
_up_ whenever you like to throw away your _qnx710-dev_ container and re-create
it from committed base image; this gives great flexibility.
