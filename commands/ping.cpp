/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ping.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: liliu <liliu@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/03 16:22:01 by liliu             #+#    #+#             */
/*   Updated: 2026/02/04 16:17:48 by liliu            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/channel.hpp"
#include "../include/client.hpp"
#include "../include/commands.hpp"
#include "../include/server.hpp"

void	handle_ping(Server *server, Client *client,
		std::vector<std::string> &args)
{
	if (args.empty())
	{
		client->send_reply(":" + server->getServerName() + " PONG "
			+ server->getServerName());
	}
	else
	{
		client->send_reply(":" + server->getServerName() + " PONG "
			+ server->getServerName() + " :" + args[0]);
	}
}
