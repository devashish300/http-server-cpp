#include "request_handler.h"
#include <sys/socket.h>
#include <iostream>
#include <cstring>
#include <regex>
#include <unistd.h>
#include <cstdlib>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <zlib.h>
#include <string>

void handleRootRoute(int client_fd)
{
    std::string responseBody = "<html><body><h1>But In The End, Every Death Is Just A New Beginning.</h1></body></html>";
    std::string response = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: " + std::to_string(responseBody.size()) + "\r\n"
    "\r\n" +
    responseBody;

    send(client_fd, response.c_str(), response.size(), 0);
}

void handleEchoRoute(int client_fd, std::string &path, char *buffer, int bytesReceived)
{
    std::string request;
    for (int i = 0; i < bytesReceived; i++)
    {
        request += buffer[i];
    }

    size_t encodingHeaderPos = request.find("Accept-Encoding:");
    bool acceptsGzip = false;

    if (encodingHeaderPos != std::string::npos)
    {
        size_t encodingHeaderEndPos = request.find("\r\n", encodingHeaderPos);

        if (encodingHeaderEndPos != std::string::npos)
        {
            std::string encodingHeader = request.substr(encodingHeaderPos, encodingHeaderEndPos - encodingHeaderPos);
            acceptsGzip = encodingHeader.find("gzip") != std::string::npos;
        }
    }

    if (acceptsGzip)
    {
        std::string content = path.substr(6);

        z_stream zs;
        memset(&zs, 0, sizeof(zs));

        if (deflateInit2(&zs, Z_BEST_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
        {
            throw(std::runtime_error("deflateInit failed while compressing."));
        }
        zs.next_in = (Bytef *)content.data();
        zs.avail_in = content.size();

        int ret;
        char outbuffer[32768];
        std::string compressedContent;

        do
        {
            zs.next_out = reinterpret_cast<Bytef *>(outbuffer);
            zs.avail_out = sizeof(outbuffer);

            ret = deflate(&zs, Z_FINISH);

            if (compressedContent.size() < zs.total_out)
            {
                compressedContent.append(outbuffer, zs.total_out - compressedContent.size());
            }
        } while (ret == Z_OK);

        deflateEnd(&zs);
        if (ret != Z_STREAM_END)
        {
            std::ostringstream oss;
            oss << "Exception during zlib compression: (" << ret << ") " << zs.msg;
            throw(std::runtime_error(oss.str()));
        }

        std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Encoding: gzip\r\nContent-Length: " + std::to_string(compressedContent.size()) + "\r\n\r\n" + compressedContent;
        send(client_fd, response.c_str(), response.size(), 0);
    }
    else
    {
        std::string content = path.substr(6);
        std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(content.size()) + "\r\n\r\n" + content;
        send(client_fd, response.c_str(), response.size(), 0);
    }
}

void handleUserAgentRoute(int client_fd, char *buffer, int bytesReceived)
{
    std::string request;
    for (int i = 0; i < bytesReceived; i++)
    {
        request += buffer[i];
    }
    std::string userAgentHeader = "User-Agent: ";
    size_t ua_start = request.find(userAgentHeader);

    if (ua_start != std::string::npos)
    {
        size_t ua_end = request.find("\r\n", ua_start);
        std::string userAgent = request.substr(ua_start + userAgentHeader.size(), ua_end - ua_start - userAgentHeader.size());
        std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(userAgent.size()) + "\r\n\r\n" + userAgent;
        send(client_fd, response.c_str(), response.size(), 0);
    }
    else
    {
        handleNotFound(client_fd);
    }
}

void handleNotFound(int client_fd)
{
    std::string message = "HTTP/1.1 404 Not Found\r\n\r\n";
    send(client_fd, message.c_str(), message.size(), 0);
}

void handleFilesRoute(int client_fd, char **argv, std::string &path, char *method, char *buffer)
{
    std::string directory = argv[2];
    std::string filename = path.substr(7);
    std::string filePath = directory + filename;

    if (strcmp(method, "GET") == 0)
    {
        if (std::filesystem::exists(filePath))
        {
            std::ifstream file(filePath);
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            std::string response = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: " + std::to_string(content.size()) + "\r\n\r\n" + content;
            send(client_fd, response.c_str(), response.size(), 0);
        }
        else
        {
            handleNotFound(client_fd);
        }
    }
    else if (strcmp(method, "POST") == 0)
    {
        char *body = strstr(buffer, "\r\n\r\n");
        if (body != NULL)
        {
            body += 4;

            std::ofstream file(filePath);
            file << body;
            file.close();

            std::string response = "HTTP/1.1 201 Created\r\n\r\n";
            send(client_fd, response.c_str(), response.size(), 0);
        }
        else
        {
            std::string response = "HTTP/1.1 400 Bad Request\r\n\r\n";
            send(client_fd, response.c_str(), response.size(), 0);
        }
    }
    else
    {
        handleNotFound(client_fd);
    }
}

void handleStreamRoute(int client_fd) {
    std::ifstream file("../files/BAIRI_PIYA.avi", std::ios::binary);
    if (!file.is_open()) {
        std::string error = "HTTP/1.1 404 Not Found\r\n\r\nVideo file not found";
        send(client_fd, error.c_str(), error.size(), 0);
        return;
    }

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string header =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: video/x-msvideo\r\n"
        "Content-Length: " + std::to_string(size) + "\r\n"
        "Connection: close\r\n\r\n";

    send(client_fd, header.c_str(), header.size(), 0);

    const size_t bufferSize = 8192;
    char buffer[bufferSize];
    while (file.read(buffer, bufferSize) || file.gcount() > 0) {
        send(client_fd, buffer, file.gcount(), 0);
    }
}

void handleClient(int client_fd, char **argv, int argc)
{
    char buffer[4096];
    int bytesReceived = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    std::cout << "Received: " << buffer << std::endl;

    char bufferCopy[4096];
    strcpy(bufferCopy, buffer);

    char *method = strtok(bufferCopy, " ");
    std::cout << "Method: " << method << std::endl;

    std::string path = strtok(NULL, " ");
    std::cout << "Path: " << path << std::endl;

    std::regex pattern("^/echo/.+$");
    std::regex filesPattern("^/files/[^/]+$");

    if (path == "/")
    {
        handleRootRoute(client_fd);
    }
    else if (std::regex_match(path, pattern))
    {
        handleEchoRoute(client_fd, path, buffer, bytesReceived);
    }
    else if (path == "/user-agent")
    {
        handleUserAgentRoute(client_fd, buffer, bytesReceived);
    }
    else if(path == "/stream"){
        handleStreamRoute(client_fd);
    }
    else if (std::regex_match(path, filesPattern) and argc == 3 && strcmp(argv[1], "--directory") == 0)
    {
        handleFilesRoute(client_fd, argv, path, method, buffer);
    }
    else
    {
        handleNotFound(client_fd);
    }
    close(client_fd);
}