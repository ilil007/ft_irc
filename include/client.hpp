/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: liliu <liliu@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/03 15:54:23 by liliu             #+#    #+#             */
/*   Updated: 2026/02/04 16:15:56 by liliu            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <sys/socket.h>

class Client{
   public :
        Client(int cfd, int cport, const std::string& cip);
        ~Client();
        int getfd() const;
        int getport() const;
        std::string getname() const;
        std::string getUsername() const;
        std::string getip() const;
        std::string &getBuffer();
        bool isRegistered() const;
        bool hasValidPassword() const;
        void setHasValidPassword(bool value);
        void setNickname(const std::string &nick);
        void setUsername(const std::string &user);
        void setRegistered(bool value);
        void send_reply(const std::string &message);
        void setFd(int new_fd);
        
    private :
        Client();
        int fd;
        int port;
        std::string ip;
        std::string nickname;
        std::string username;
        std::string buffer;
        bool is_registered;
        bool has_valid_password;
};
