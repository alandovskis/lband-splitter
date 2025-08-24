FROM node:18-alpine
WORKDIR /usr/share/app
COPY web/ ./
RUN npm install -g http-server
EXPOSE 80
CMD ["http-server", ".", "-p", "80"]
