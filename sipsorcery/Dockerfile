FROM mcr.microsoft.com/dotnet/sdk:5.0 AS build
WORKDIR /src
COPY ["./sipsorcery/server", ""]
RUN dotnet publish "webrtc-echo.csproj" -c Release -o /app/publish
WORKDIR /src/client
COPY ["./sipsorcery/client", ""]
RUN dotnet publish "webrtc-echo-client.csproj" -c Release -o /app/publish/client

FROM mcr.microsoft.com/dotnet/runtime:5.0 AS final
WORKDIR /app
EXPOSE 8080
#EXPOSE 49000-65535
COPY ["html", "../html/"]
COPY ["./sipsorcery/client.sh", "/client.sh"]
RUN chmod +x /client.sh
COPY --from=build /app/publish .
COPY --from=build /app/publish/client .
ENTRYPOINT ["dotnet", "webrtc-echo.dll"]
