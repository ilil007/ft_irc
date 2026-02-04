# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: liliu <liliu@student.42.fr>                +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/02/03 15:47:18 by liliu             #+#    #+#              #
#    Updated: 2026/02/04 16:25:37 by liliu            ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = ircserv

CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++98

SRCS =  src/main.cpp \
        src/server.cpp \
        src/client.cpp \
        src/channel.cpp \
		commands/cap.cpp \
		commands/join.cpp \
		commands/nick.cpp \
		commands/pass.cpp \
		commands/privmsg.cpp \
		commands/quit.cpp \
		commands/user.cpp \
		commands/ping.cpp \
		commands/invite.cpp \
		commands/topic.cpp \
		commands/kick.cpp \
		commands/mode.cpp \
		commands/whois.cpp \
		commands/part.cpp \
		commands/who.cpp

		
OBJS = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re