#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "helpers.hpp"
#include "requests.hpp"
#include "json/single_include/nlohmann/json.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <cstring>

using json = nlohmann::json;
using namespace std;

// Constante folosite pentru interactiunea cu serverul
#define SERVER "34.241.4.235"
#define PORT 8080
#define REGISTER "/api/v1/tema/auth/register"
#define JSON_PAYLOAD "application/json"
#define LOGIN "/api/v1/tema/auth/login"
#define ACCESS "/api/v1/tema/library/access"
#define BOOKS "/api/v1/tema/library/books"
#define LOGOUT "/api/v1/tema/auth/logout"
#define NIL ""
#define CONTENT_LENGTH "Content-Length: "
#define SETCOOKIE "Set-Cookie: "
#define TOKEN "token"
#define CONTENT_TYPE "Content-Type: "
#define HEADER_TERMINATOR "\r\n\r\n"
#define LINE_SEPARTOR "\r\n"


// Enum pentru parsarea input-urilor posibile
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

// Verifica daca un sir de caractere reprezinta un numar
bool isNumber(const string& str)
{
    return str.find_first_not_of("0123456789") == str.npos;
}

// Verifica daca un sir de caracter contine doar caractere printable
bool checkPrintable(string str)
{
    for(auto l : str)
    {
        if (!isprint(l))
        {
            return false;
        }
        
    }
    return true;
}

// Verifica codul unui mesaj HTTP
uint32_t check_code(string response)
{
    string codeString = response.substr(9, 3);
    int code = stoi(codeString);
    if (code >= 400)
    {
        return code;
    }
    
    return 0;
}

// Verifica daca utilizatorul este logat (exista un cookie)
bool checkConnected(string cookie)
{
    if(cookie == NIL)
    {
        cout << "You need to log in first!\n";
        return false;
    }
    return true;
}

// Verifica daca utilizatorul este autentificat
bool checkAuth(string token)
{
    if(token == NIL)
    {
        cout << "You need to enter the library first!\n";
        return false;
    }
    return true;
}

// Extrage Content size dintr-un mesaj HTTP
int get_content_size(char *response)
{
    int content_size = 0;
    string message(response);
    if (message.find(CONTENT_LENGTH) != message.npos)
    {
        int line_begin = message.find(CONTENT_LENGTH) + strlen(CONTENT_LENGTH);
        string content_length = message.substr(line_begin, message.length());
        content_length = content_length.substr(0, content_length
                                                .find_first_of(LINE_SEPARTOR));
        content_size = stoi(content_length);
    }
    return content_size;
}

// Extrage Content type pentru un mesaj HTTP
string get_content_type(char *response)
{
    string content(NIL);
    string message(response);
    if (message.find(CONTENT_TYPE) != message.npos)
    {
        int line_begin = message.find(CONTENT_TYPE) + strlen(CONTENT_TYPE);
        content = message.substr(line_begin, message.length());
        content = content.substr(0, content.find_first_of(";"));
    }
    return content;
}

// Obtine content-ul unui mesaj HTTP
string get_content(char *response)
{
    string message(response);
    int content_size = get_content_size(response);
    if (content_size != 0 && message.find(HEADER_TERMINATOR) != message.npos)
    {
        message = message.substr(message.find(HEADER_TERMINATOR) + 4);
        return message;
    }
    return NIL;
}

// Obtine o valoare dintr-un content de tipul JSON al unui content HTTP
string get_content_value(char *response, string valueInContent)
{
    int content_size = get_content_size(response);
    string message(response);
    string content_type = get_content_type(response);
    if (content_size != 0 && message.find(HEADER_TERMINATOR) != message.npos)
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
    return NIL;
}

// Obtine un COOKIE dintr-un raspuns http de tipul connect.sid
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
    return NIL;
}

// Afiseaza continutul unui mesaj HTTP in format JSON pretty
void print_content_message(char *response)
{ 
    string content = get_content(response);
    if (!json::accept(content))
    {
        fprintf(stdout, "Not a valid JSON input!\n");
        return;
    }
    json array = json::parse(content);
    cout << array.dump(4) << endl;
}

// Trateaza cazurile de eroare pentru un raspuns HTTP
int handle_response(char *response)
{
    string response_string(response);
    uint32_t error_code = check_code(response_string);
    if (error_code == 429)
    {
        cout << "Server is having too many requests, try again later!\n";
        return 0;
    }
    if (error_code)
    {
        string error = get_content_value(response, "error");
        if (error != NIL)
        {
            fprintf(stdout, "%s\n", error.c_str());
            
        }
        else
        {
            string content = get_content(response);
            cout << "Error: " << error_code << endl;
            cout << content;
        }
        return 0;
    }
    

    return 1;
}

// Trimite o cerere de inregistrare de tipul HTTP
void send_register(string cookie)
{
    // Verificam ca utilizatorul nu este deja logat
    if (cookie != NIL)
    {
        cout << "You need to logout first!\n";
        return;
    }
    
    // Obtinem datele contului ce trebuie creeat
    char *message;
    char *response;
    string username, password;
    cout << "username=";
    cin >> username;
    if (!checkPrintable(username))
    {
        cout << "Try again and enter a valid printable username!\n";
        return;
    }
    cout << "password=";
    cin >> password;
    if (!checkPrintable(password))
    {
        cout << "Try again and enter a valid printable password!\n";
        return;
    }
    
    // Creem cererea HTTP si il trimitel
    json j;
    j["username"] = username;
    j["password"] = password;
    message = compute_post_request(SERVER, REGISTER, JSON_PAYLOAD,
                                    j.dump(4).c_str(), NULL, NULL);
    int sockfd = open_connection(SERVER, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close(sockfd);

    // Verificam raspunsul
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

// Trimite o cerere de logare de tipul HTTP
string send_login(string checkAlreadyLoginCookie)
{
    // Verificam daca nu suntem deja logat
    if (checkAlreadyLoginCookie != NIL)
    {
        cout << "You need to logout first!\n";
        return checkAlreadyLoginCookie;
    }
    
    // Obtinem datele utilizatorul
    char *message;
    char *response;
    string username, password;
    cout << "username=";
    cin >> username;
    if (!checkPrintable(username))
    {
        cout << "Try again and enter a valid printable username!\n";
        return NIL;
    }
    
    cout << "password=";
    cin >> password;
    if (!checkPrintable(password))
    {
        cout << "Try again and enter a valid printable password!\n";
        return NIL;
    }

    // Creem cererea HTTP si o trimitem
    json j;
    j["username"] = username;
    j["password"] = password;
    message = compute_post_request(SERVER, LOGIN, JSON_PAYLOAD,
                                    j.dump(4).c_str(), NULL, NULL);
    int sockfd = open_connection(SERVER, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close(sockfd);

    // Verificam raspunsul HTTP
    if (!handle_response(response))
    {
        free(message);  
        free(response);
        return NIL;
    }
    string cookie = get_cookie(response);
    cout << "200 - OK - Bun venit!\n";

    free(message);
    free(response);

    return cookie;
}

// Intra in librarie de carti / Obtine token-ul de autentificare
string enter_library(string cookie, string inputToken)
{
    // Verificam daca utilizator este conectat
    if (!checkConnected(cookie))
    {
        return NIL;
    }
    // Verificam daca utilizator nu este deja autorizat
    if (inputToken != NIL)
    {
        cout << "You already entered the library!\n";
        return inputToken;
    }
    
    // Trimitem mesajul HTTP;
    char *message;
    char *response;
    message = compute_get_request(SERVER, ACCESS, cookie.c_str(), NULL, NULL);
    int sockfd = open_connection(SERVER, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close(sockfd);

    // Verifica raspunsul
    if (!handle_response(response))
    {
        free(message);
        free(response);
        return NIL;
    }
    
    string token = get_content_value(response, TOKEN);
    cout << "OK - 200 - Librarie accessata cu succes!\n";
    free(message);
    free(response);
    return token;
}

// Obtine lista de carti 
void get_books(string cookie, string token)
{   
    // Verificam daca utilizatorul este conectat si autentificat
    if (!checkConnected(cookie) || !checkAuth(token))
    {
        return;
    }


    // Trimite cererea HTTP
    char *message;
    char *response;
    message = compute_get_request(SERVER, BOOKS, NULL, token.c_str(), NULL);
    int sockfd = open_connection(SERVER, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close(sockfd);

    // Verifica raspunsul
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

// Citeste datele necesare unei carti si intoarce obiectul JSON
json create_book()
{
    json j;
    string input;
    cout << "title=";
    cin >> input;
    if (!checkPrintable(input))
    {
        cout << "Try again and enter a valid printable title!\n";
        return NIL;
    }
    j["title"] = input;
    cout << "author=";
    cin >> input;
    if (!checkPrintable(input))
    {
        cout << "Try again and enter a valid printable author!\n";
        return NIL;
    }
    j["author"] = input;
    cout << "genre=";
    cin >> input;
    if (!checkPrintable(input))
    {
        cout << "Try again and enter a valid printable genre!\n";
        return NIL;
    }
    j["genre"] = input;
    cout << "publisher=";
    cin >> input;
    if (!checkPrintable(input))
    {
        cout << "Try again and enter a valid printable publisher!\n";
        return NIL;
    }
    j["publisher"] = input;
    cout << "page_count=";
    cin >> input;

    if (!isNumber(input))
    {
        cout << "page_count must be a number! Try again!\n";
        return NIL;
    }
    
    j["page_count"] = input;

    return j;
}

// Trimite o cerere de adaugare a unei carti
void add_book(string cookie, string token)
{
    // Verificam daca utilizatorul este conectat si autentificat
    if (!checkConnected(cookie) || !checkAuth(token))
    {
        return;
    }

    // Trimitem cererea HTTP
    char *message;
    char *response;
    json book = create_book();
    if (book == NIL)
    {
        return;
    }
    message =  compute_post_request(SERVER, BOOKS, JSON_PAYLOAD,
                            book.dump(4).c_str(), NULL, token.c_str());
    int sockfd = open_connection(SERVER, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close(sockfd);
    
    // Verificam raspunsul
    if (!handle_response(response))
    {
        free(message);
        free(response);
        return;
    }

    cout << "OK - 200 - Cartea a fost adaugata!\n";

    free(message);
    free(response);
}

// Obtine o carte 
void get_book(string cookie, string token)
{
    // Verificam daca utilizatorul este conectat si autentificat
    if (!checkConnected(cookie) || !checkAuth(token))
    {
        return;
    }

    // Trimitem cererea HTTP
    char *message;
    char *response;
    string bookId;
    cout << "id=";
    cin >> bookId;
    if (!isNumber(bookId))
    {
        cout << "Id must be a number! Try again!\n";
        return;
    }
    message = compute_get_request(SERVER, BOOKS, NULL,
                                    token.c_str(), bookId.c_str());
    int sockfd = open_connection(SERVER, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close(sockfd);

    // Verificam raspunsul
    if (!handle_response(response))
    {
        free(message);
        free(response);
        cout << "Enter a valid ID! Try again!\n";
        return;
    }
    print_content_message(response);

    free(message);
    free(response);
}

// Sterge o carte
void delete_book(string cookie, string token)
{
    // Verificam daca utilizatorul este conectat si autentificat
    if (!checkConnected(cookie) || !checkAuth(token))
    {
        return;
    }

    // Trimitem cererea HTTP
    char *message;
    char *response;
    string bookId;
    cout << "id=";
    cin >> bookId;
    if (!isNumber(bookId))
    {
        cout << "Id must be a number! Try again!\n";
        return;
    }
    message = compute_delete_request(SERVER, BOOKS, NULL,
                                        token.c_str(), bookId.c_str());
    int sockfd = open_connection(SERVER, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close(sockfd);

    // Verificam raspunsul
    if (!handle_response(response))
    {    
        free(message);
        free(response);
        cout << "Enter a valid ID!\n";
        return;
    }
    cout << "OK - 200 - Cartea a fost stearsa!\n";
    
    free(message);
    free(response);

}

// Delogheaza un utilizator
void logout(string cookie)
{
    // Verifica daca utilizatorul este conectat
    if (!checkConnected(cookie))
    {
        return;
    }

    // Trimite cererea HTTp
    char *message;
    char *response;
    message = compute_get_request(SERVER, LOGOUT, cookie.c_str(), NULL, NULL);
    int sockfd = open_connection(SERVER, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close(sockfd);

    // Verifica raspunsul
    if (!handle_response(response))
    {
        free(message);
        free(response);
        return;
    }
    cout << "OK - 200 - Logout executat cu succes!\n";
    free(message);
    free(response);
}

// Pentru un sir de caractere de tipul input, intoarce intrarea corecta in enum
inputs get_input_value(const string input)
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
    string cookie(NIL);
    string token(NIL);

    // Citim datele la pana la introducerea comenzii exit
    while (1)
    {   
        cin >> command;
        if(get_input_value(command) == eExit) 
        {
            break;
        }
        switch (get_input_value(command))
        {
            case eRegister:
                send_register(cookie);
                break;
            case eLogin:
                cookie = send_login(cookie);
                break;
            case eEnterLibrary:
                token = enter_library(cookie, token);
                break;
            case eGetBooks:
                get_books(cookie, token);
                break;
            case eGetBook:
                get_book(cookie, token);
                break;
            case eAddBook:
                add_book(cookie, token);
                break;
            case eDeleteBook:
                delete_book(cookie, token);
                break;
            case eLogout:
                logout(cookie);
                cookie = NIL;
                token = NIL;
                break;
            case eInvalidInput:
                cout << "Enter a valid command!\n";
                break;   
            default:
                break;
        }
    }
    
    return 0;
}
