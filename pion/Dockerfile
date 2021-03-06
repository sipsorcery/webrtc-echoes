FROM golang:latest AS build
WORKDIR /src
COPY ["pion", ""]
RUN go build main.go
WORKDIR /src/client
RUN go build -o client main.go

FROM golang:latest AS final
WORKDIR /app
COPY --from=build /src/main .
COPY --from=build /src/client/client .
COPY ["html", "../html/"]
COPY ["./pion/client.sh", "/client.sh"]
RUN chmod +x /client.sh
EXPOSE 8080
ENTRYPOINT /app/main