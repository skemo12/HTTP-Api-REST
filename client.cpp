#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.hpp"
#include "requests.hpp"
#include "json/single_include/nlohmann/json.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>

using json = nlohmann::json;
using namespace std;

#define SERVER "34.241.4.235"
#define PORT 8080
#define REGISTER "/api/v1/tema/auth/register"
#define JSON_PAYLOAD "application/json"
#define LOGIN "/api/v1/tema/auth/login"
#define ACCESS "/api/v1/tema/library/access"
#define BOOKS "/api/v1/tema/library/books"
#define LOGOUT "/api/v1/tema/auth/logout"
#define NOVALUESTRING ""
#define CONTENT_LENGTH "Content-Length: "
#define SETCOOKIE "Set-Cookie: "
#define TOKEN "token"
#define CONTENT_TYPE "Content-Type: "

enum inputs {
    eRegister,
    eLogin,
    eEnterLibrary,
    eGetBooks,
    eGetBook,
    eAddBook,
    eDeleteBook,
    eLogout,
    eExit,
    eInvalidInput
};

bool isNumber(const string& str)
{
    return str.find_first_not_of("0123456789") == str.npos;
}

uint32_t bad_result(string response)
{
    string codeString = response.substr(9, 3);
    int code = stoi(codeString);
    if (code >= 400)
    {
        return code;
    }
    
    return 0;
}

int get_content_size(char *response)
{
    int content_size = 0;
    string message(response);
    if (message.find(CONTENT_LENGTH) != message.npos)
    {
        int line_begin = message.find(CONTENT_LENGTH) + strlen(CONTENT_LENGTH);
        string content_length = message.substr(line_begin, message.length());
        content_length = content_length.substr(0, content_length
                                                    .find_first_of("\r\n"));
        content_size = stoi(content_length);
    }
    return content_size;
}

string get_content_type(char *response)
{
    string content(NOVALUESTRING);
    string message(response);
    if (message.find(CONTENT_TYPE) != message.npos)
    {
        int line_begin = message.find(CONTENT_TYPE) + strlen(CONTENT_TYPE);
        content = message.substr(line_begin, message.length());
        content = content.substr(0, content.find_first_of(";"));
    }
    return content;
}

string get_content(char *response)
{
    string message(response);
    int content_size = get_content_size(response);
    if (content_size != 0 && message.find("\r\n\r\n") != message.npos)
    {
        message = message.substr(message.find("\r\n\r\n") + 4);
        return message;
    }
    return NOVALUESTRING;
}

string get_content_value(char *response, string valueInContent)
{
    int content_size = get_content_size(response);
    string message(response);
    string content_type = get_content_type(response);
    if (content_size != 0 && message.find("\r\n\r\n") != message.npos)
    {
        string content = get_content(response);
        if (json::accept(content) && content_type == JSON_PAYLOAD)
        {
            json j = json::parse(content);
            if (j.find(valueInContent) != j.end())
            {
                string value =  j.at(valueInContent).dump(4);
                value = value.substr(1, value.length() - 2);
                return value;
            }
        }
        
        
    }
    return NOVALUESTRING;
}

string get_cookie(char *response)
{
    string message(response);
    if (message.find(SETCOOKIE) != message.npos)
    {
        string cookie = message.substr(message.find(SETCOOKIE));
        int endpos = cookie.find_first_of(";");
        cookie = cookie.substr(12, endpos - 12);
        return cookie;
    }
    return NOVALUESTRING;
}

void print_content_message(char *response)
{
   
    
    string content = get_content(response);
    if (!json::accept(content))
    {
        fprintf(stderr, "Not a valid JSON input!\n");
        return;
    }
    json array = json::parse(content);
    cout << array.dump(4) << endl;
}

int handle_response(char *response)
{
    string response_string(response);
    if (bad_result(response_string))
    {
        string error = get_content_value(response, "error");
        if (error != NOVALUESTRING)
        {
            fprintf(stderr, "%s\n", error.c_str());
            return 0;
        }
        
    }


    return 1;
}

void send_register()
{
    char *message;
    char *response;
    string username, password;
    cout << "username=";
    cin >> username;
    cout << "password=";
    cin >> password;
    json j;
    j["username"] = username;
    j["password"] = password;
    message = compute_post_request(SERVER, REGISTER, JSON_PAYLOAD, j.dump(4).c_str(), 0, NULL, 0, NULL);
    // cout << message;
    int sockfd = open_connection(SERVER, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close(sockfd);

    if (!handle_response(response))
    {
        free(message);
        free(response);
        return;
    }
    
    
    cout << "OK - 200 - Utilizator înregistrat cu succes!\n";
    free(message);
    free(response);

}

// returns cookie
string send_login()
{
    char *message;
    char *response;
    string username, password;
    cout << "username=";
    cin >> username;
    cout << "password=";
    cin >> password;
    json j;
    j["username"] = username;
    j["password"] = password;
    message = compute_post_request(SERVER, LOGIN, JSON_PAYLOAD, j.dump(4).c_str(), 0, NULL, 0, NULL);
    // cout << "Sending:\n" << message;
    int sockfd = open_connection(SERVER, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    string cookie = get_cookie(response);

    if (!handle_response(response))
    {
        free(message);  
        free(response);
        return NOVALUESTRING;
    }

    cout << "200 - OK - Bun venit!\n";

    free(message);
    free(response);

    return cookie;
}

string enter_library(string cookie)
{
    char *message;
    char *response;
    if (cookie == NOVALUESTRING)
    {
        fprintf(stderr, "You need to log in first!\n");
        return NOVALUESTRING;
    }
    
    message = compute_get_request(SERVER, ACCESS, NULL, cookie.c_str(), 1, NULL, NULL);
    int sockfd = open_connection(SERVER, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    
    response = receive_from_server(sockfd);
   
    
    string response_string(response);


    if (!handle_response(response))
    {
        free(message);
        free(response);
        return NOVALUESTRING;
    }
    
    string token = get_content_value(response, TOKEN);
    
    cout << "OK - 200 - Librarie accessata cu succes!\n";
    free(message);
    free(response);
    return token;
}

void get_books(string token)
{
    // if(token == NOVALUESTRING)
    // {
    //     fprintf(stderr, "No authentification token\n");
    //     return;
    // }
    char *message;
    char *response;
    message = compute_get_request(SERVER, BOOKS, NULL, NULL, 1, token.c_str(), NULL);
    // cout << message;
    int sockfd = open_connection(SERVER, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    
    response = receive_from_server(sockfd);

    if (!handle_response(response))
    {
        free(message);
        free(response);
        return;
    }

    print_content_message(response);

    free(message);
    free(response);
}

json create_book()
{
    json j;
    string input;
    cout << "title=";
    cin >> input;
    j["title"] = input;
    cout << "author=";
    cin >> input;
    j["author"] = input;
    cout << "genre=";
    cin >> input;
    j["genre"] = input;
    cout << "publisher=";
    cin >> input;
    j["publisher"] = input;
    cout << "page_count=";
    cin >> input;

    if (!isNumber(input))
    {
        fprintf(stderr, "page_count must be a number! Try again!\n");
        return NOVALUESTRING;
    }
    
    j["page_count"] = input;

    return j;
}

void add_book(string token)
{
    if(token == NOVALUESTRING)
    {
        fprintf(stderr, "You need to first enter the library!\n");
        return;
    }
    char *message;
    char *response;
    json book = create_book();
    if (book == NOVALUESTRING)
    {
        return;
    }
    
    message =  compute_post_request(SERVER, BOOKS, JSON_PAYLOAD,
                            book.dump(4).c_str(), 0, NULL, 0, token.c_str());
    // cout << message;
    int sockfd = open_connection(SERVER, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    cout << response << endl;

    if (!handle_response(response))
    {
        free(message);
        free(response);
        return;
    }

    // print_content_message(response);
    cout << "OK - 200 - Cartea a fost adaugata!\n";

    free(message);
    free(response);
}

void get_book(string token)
{
    // if(token == NOVALUESTRING)
    // {
    //     fprintf(stderr, "No authentification token\n");
    //     return;
    // }
    char *message;
    char *response;
    string bookId;
    cout << "id=";
    cin >> bookId;
    message = compute_get_request(SERVER, BOOKS, NULL, NULL, 1, token.c_str(), bookId.c_str());
    // cout << message;
    int sockfd = open_connection(SERVER, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    // cout << message << endl;
    response = receive_from_server(sockfd);
    // cout << response << endl;
    if (!handle_response(response))
    {
        free(message);
        free(response);
        return;
    }
    print_content_message(response);

    free(message);
    free(response);
}

void delete_book(string token)
{
    // if(token == NOVALUESTRING)
    // {
    //     fprintf(stderr, "No authentification token\n");
    //     return;
    // }
    char *message;
    char *response;
    string bookId;
    cout << "id=";
    cin >> bookId;
    message = compute_delete_request(SERVER, BOOKS, NULL, NULL, 1, token.c_str(), bookId.c_str());
    // cout << message;
    int sockfd = open_connection(SERVER, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    if (!handle_response(response))
    {    
        free(message);
        free(response);
        return;
    }
    print_content_message(response);
    
    free(message);
    free(response);

}

void logout(string cookie)
{
    if (cookie == NOVALUESTRING)
    {
        fprintf(stderr, "Not logged in\n");
        return;
    }
    char *message;
    char *response;
    message = compute_get_request(SERVER, LOGOUT, NULL, cookie.c_str(), 1, NULL, NULL);
    // cout << message;
    int sockfd = open_connection(SERVER, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    if (!handle_response(response))
    {
        free(message);
        free(response);
        return;
    }
    
    free(message);
    free(response);
}

inputs get_value(const string input)
{
    if (input == "register") return eRegister;
    if (input == "login") return eLogin;
    if (input == "enter_library") return eEnterLibrary;
    if (input == "get_books") return eGetBooks;
    if (input == "get_book") return eGetBook;
    if (input == "add_book") return eAddBook;
    if (input == "delete_book") return eDeleteBook;
    if (input == "logout") return eLogout;
    if (input == "exit") return eExit;
    return eInvalidInput;
}

int main(int argc, char *argv[])
{

    string command;
    string cookie(NOVALUESTRING);
    string token(NOVALUESTRING);

    while (1)
    {   
        cin >> command;
        if(get_value(command) == eExit) 
        {
            break;
        }
        switch (get_value(command))
        {
            case eRegister:
                send_register();
                break;
            case eLogin:
                cookie = send_login();
                break;
            case eEnterLibrary:
                token = enter_library(cookie);
                break;
            case eGetBooks:
                get_books(token);
                break;
            case eGetBook:
                get_book(token);
                break;
            case eAddBook:
                add_book(token);
                break;
            case eDeleteBook:
                delete_book(token);
                break;
            case eLogout:
                logout(cookie);
                cookie = NOVALUESTRING;
                token = NOVALUESTRING;
                break;
            case eInvalidInput:
                fprintf(stderr, "Not a valid input\n");
                break;   
            default:
                break;
        }
    
        
    }
    
    return 0;
}
