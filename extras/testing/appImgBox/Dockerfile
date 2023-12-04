# Use an official Node.js runtime as a parent image
FROM ubuntu:22.04

# LABEL about the custom image
LABEL maintainer="victor@home.com"
LABEL version="0.1"
LABEL description="This is a custom Docker Image AppImage sandboxing"

RUN apt-get update
RUN apt-get install -y libfuse2


# Define the command to run the application
COPY PolyVR-x86_64.AppImage /PolyVR-x86_64.AppImage
CMD ["./PolyVR-x86_64.AppImage", "--help"]
