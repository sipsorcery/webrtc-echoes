FROM node:latest
WORKDIR /app
COPY ["werift", ""]
COPY ["html", "../html/"]
COPY ["werift/client.sh", "/client.sh"]
RUN chmod +x /client.sh
RUN npm install
EXPOSE 8080
ENTRYPOINT npm run server
