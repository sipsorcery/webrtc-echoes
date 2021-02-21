FROM node:latest
WORKDIR /app
COPY ["werift", ""]
COPY ["html", "../html/"]
RUN npm install
EXPOSE 8080
ENTRYPOINT npm run server
