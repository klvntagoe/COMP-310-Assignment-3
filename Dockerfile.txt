FROM ubuntu:latest
LABEL 	Author="Shabirmean" \
      	Email="shabirmean@cs.mcgill.ca" \
      	Institution="McGill University" \
      	Department="School of Computer Science (SOCS)" \
      	Description="Dockerfile to provide execution environment for COMP310 Assignment-3 (Fall-2018 : Prof. M.Maheshwaran)" \
	Date="15th Nov 2018"

RUN apt-get update
RUN apt-get install -y 	gcc \
			wget \
			vim \
			vis \
			sudo \
			libseccomp2 \
			libseccomp-dev \
			libcap-dev \
			iputils-ping \
			net-tools \
			netcat \
			stress \
			gdb \
			htop \
			debootstrap \
			psmisc \
			uidmap \
			&& apt-get clean

#RUN useradd -m comp310-user && echo "comp310-user:comp310" | chpasswd && adduser comp310-user sudo
#USER comp310-user
WORKDIR /home
ENTRYPOINT ["/bin/bash", "-c", "trap : TERM INT; sleep infinity & wait"]

# docker build . -t smean_image_comp310
# --------------------------------------------------------------
#	         Run your container as follows
#	          (Give it a meaningful name)
# --------------------------------------------------------------
# docker run --privileged -d --rm --name=<MEANINGFUL_NAME> smean_image_comp310
# eg: docker run --privileged -d --rm --name=shabirmean smean_image_comp310

# --------------------------------------------------------------
#             Get inside your container as follows
#  (Basically your running a shell process in the container namespace)
# --------------------------------------------------------------
# docker exec -it <MEANINGFUL_NAME> /bin/bash
# eg: docker exec -it shabirmean /bin/bash