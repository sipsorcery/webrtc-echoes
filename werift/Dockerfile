FROM node:latest
WORKDIR /app
COPY ["werift", ""]
COPY ["html", "../html/"]
COPY ["werift/client.sh", "/client.sh"]
RUN apt-get -y update && \
    apt-get -y upgrade && \
    apt-get -y install libgstreamer1.0-0 gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-doc gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-pulseaudio
RUN chmod +x /client.sh
RUN npm install
EXPOSE 8080
ENTRYPOINT npm run server
