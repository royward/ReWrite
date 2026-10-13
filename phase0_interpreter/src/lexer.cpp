// ============================================================================
// File: lexer.cpp
// Description: Lexing - takes a string_view and returns a list of Tokens
// Also, Token.to_string()
// ============================================================================
// Copyright 2026 Roy Ward
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdexcept>
#include <sstream>
#include <iostream>
#include "token.hpp"

std::string lex_string(std::string_view code, std::size_t& p, char term) {
    std::string result;
    std::size_t len=code.length();
    while(p<len) {
        char c=code[p];
        p++;
        if(c==term) {
            return result;
        }
        if(c=='\\' && p<len) {
            char esc=code[p];
            p++;
            switch(esc) {
                case 't': c='\t'; break;
                case 'r': c='\r'; break;
                case 'n': c='\n'; break;
                case '0': c='\0'; break;
                default: c=esc;
            }
        }
        result+=c;
    }
    std::ostringstream msg;
    msg << "Unterminated " << term << "string";
    throw std::runtime_error(msg.str());
}

std::vector<Token> lex(uint32_t file_id, std::string_view code) {
    std::vector<Token> result;
    std::size_t len=code.length();
    std::size_t p=0;
    std::size_t offset_start_of_row=0;
    uint32_t row=0;
    while(p<len) {
        TokenKind token_kind;
        bool use_transformed_string=false;
        std::string transformed_string;
        std::size_t start_p=p;
        //std::cout << code[p] << std::flush;
        char c=code[p++];
        if(c=='_' || c=='@' || std::isalpha(static_cast<unsigned char>(c))) {
            while(p<len && (code[p]=='_' || code[p]=='@' || std::isalpha(static_cast<unsigned char>(code[p])) || std::isdigit(static_cast<unsigned char>(code[p])))) {
                p++;
            }
            if(c=='_' && p==start_p+1) {
                token_kind=Wildcard;
            } else {
                std::string_view sub=code.substr(start_p, p-start_p);
                if(sub=="true") {
                    token_kind=True;
                } else if(sub=="false") {
                    token_kind=False;
                } else if(sub=="const") {
                    token_kind=ConstToken;
                } else if(sub=="match") {
                    token_kind=Match;
                } else if(sub=="update") {
                    token_kind=Update;
                } else if(sub=="@match") {
                    token_kind=MatchDecline;
                } else if(sub=="@update") {
                    token_kind=UpdateDecline;
                } else if(sub=="when") {
                    token_kind=When;
                } else if(sub=="type" || sub=="fn" || sub=="export" || sub=="eof_compiler") {
                    while(p<len && code[p]!=';') {
                        p++;
                    }
                    if(p<len)p++;
                    continue; // ignoring type information
                } else {
                    token_kind=Identifier;
                }
            }
        } else if(std::isdigit(static_cast<unsigned char>(c))) {
           while(p<len && std::isdigit(static_cast<unsigned char>(code[p]))) {
                p++;
            }
            token_kind=UnsignedInteger;
        } else if(std::isblank(static_cast<unsigned char>(c))) {
            continue; // NOTE: no token produced for whitespaces
        } else {
            switch(c) {
                case '\n':{
                    row++;
                    offset_start_of_row=p;
                    continue; // NOTE: no token produced for newline
                }
                case '{':token_kind=LBrace; break;
                case '}':token_kind=RBrace; break;
                case ',':token_kind=Comma; break;
                case '(':token_kind=LParen; break;
                case ')':token_kind=RParen; break;
                case ';':token_kind=Semicolon; break;
                case '*':token_kind=Star; break;
                case '+':token_kind=Plus; break;
                case '~':token_kind=Tilda; break;
                case ':':token_kind=Colon; break;
                case '.':token_kind=Dot; break;
                case '%':token_kind=Modulus; break;
                case '^':token_kind=Xor; break;
                case '/': {
                    if(p<len && code[p]=='/') {
                        p++;
                        while(p<len && code[p]!='\n') {
                            p++;
                        }
                        continue; // no token produced for comments
                    } else {
                        token_kind=Divide;;
                    }
                } break;
                case '-': {
                    if(p<len && code[p]=='>') {
                        p++;
                        token_kind=Arrow;
                    } else {
                        token_kind=Minus;
                    }
                } break;
                case '<': {
                    if(p<len && code[p]=='=') {
                        p++;
                        token_kind=LessEqual;
                    } else if(p<len && code[p]=='<') {
                        p++;
                        token_kind=ShiftLeft;
                    } else {
                        token_kind=Less;
                    }
                } break;
                case '>': {
                    if(p<len && code[p]=='=') {
                        p++;
                        token_kind=GreaterEqual;
                    } else if(p<len && code[p]=='>') {
                        p++;
                        token_kind=ShiftRight;
                    } else {
                        token_kind=Greater;
                    }
                } break;
                case '!': {
                    if(p<len && code[p]=='=') {
                        p++;
                        token_kind=NotEqual;
                    } else {
                        token_kind=Not;
                    }
                } break;
                case '=': {
                    if(p<len && code[p]=='=') {
                        p++;
                        token_kind=EqualEqual;
                    } else if(p<len && code[p]=='>') {
                        p++;
                        token_kind=DoubleArrow;
                    } else {
                        token_kind=Equal;
                    }
                } break;
                case '&': {
                    if(p<len && code[p]=='&') {
                        p++;
                        token_kind=AndAnd;
                    } else {
                        token_kind=And;
                    }
                } break;
                case '|': {
                    if(p<len && code[p]=='|') {
                        p++;
                        token_kind=OrOr;
                    } else {
                        token_kind=Or;
                    }
                } break;
                case '#': {
                    while(p<len && std::isalpha(code[p])) {
                        p++;
                    }
                    std::string_view sub=code.substr(start_p, p-start_p);
                    if(sub=="#never") {
                        token_kind=HashNever;
                    } else if(sub=="#error") {
                        token_kind=HashError;
                    } else {
                        std::ostringstream msg;
                        msg << "Unexpected # term:" << sub << ", should be #error or #never";
                        throw std::runtime_error(msg.str());
                    }
                } break;
                case '\'': {
                    token_kind=Chars;
                    use_transformed_string=true;
                    transformed_string=lex_string(code,p,'\'');
                } break;
                case '\"': {
                    token_kind=String;
                    use_transformed_string=true;
                    transformed_string=lex_string(code,p,'\"');
                } break;
                default: {
                    std::ostringstream msg;
                    msg << "Unexpected character: " << c << ", row " << row+1;
                    throw std::runtime_error(msg.str());
                }
            }
        }
        result.push_back(Token{token_kind,
            use_transformed_string?transformed_string:static_cast<std::string>(code.substr(start_p, p - start_p)),
            file_id,
            row,
            static_cast<uint32_t>(start_p - offset_start_of_row),
            static_cast<uint32_t>(p - offset_start_of_row),
        });
    }
    // put a sentinel Eof at the end
    result.push_back(Token{Eof,"",file_id,row,static_cast<uint32_t>(p - offset_start_of_row),static_cast<uint32_t>(p - offset_start_of_row)});
    return result;
}

std::string Token::to_string() const {
    std::stringstream ss;
    ss << '{' << text << ':'<< static_cast<int>(kind) << ':' << row+1 << ',' << start_column+1 << '-' << end_column+1 << '}';
    return ss.str();
}

