/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   privmsg.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: liliu <liliu@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/03 16:22:05 by liliu             #+#    #+#             */
/*   Updated: 2026/02/04 16:17:36 by liliu            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/channel.hpp"
#include "../include/client.hpp"
#include "../include/commands.hpp"
#include "../include/server.hpp"

void	handle_privmsg(Server *server, Client *client,
		std::vector<std::string> &args)
{
	Channel	*channel;
	Client	*target_client;

	if (!client->isRegistered())
	{
		client->send_reply(":" + server->getServerName() + " 451 "
			+ client->getname() + " :You have not registered");
		return ;
	}
	if (args.size() < 2)
	{
		if (args.empty())
			client->send_reply(":" + server->getServerName() + " 411 "
				+ client->getname() + " :No recipient given (PRIVMSG)");
		else
			client->send_reply(":" + server->getServerName() + " 412 "
				+ client->getname() + " :No text to send");
		return ;
	}
	std::string target_name = args[0];
	std::string message = args[1];
	std::string full_message = ":" + client->getname() + " PRIVMSG "
		+ target_name + " :" + message;
	if (target_name[0] == '#')
	{
		channel = server->getChannel(target_name);
		if (!channel)
			client->send_reply(":" + server->getServerName() + " 403 "
				+ client->getname() + " " + target_name + " :No such channel");
		else if (!channel->isClientInChannel(client))
			client->send_reply(":" + server->getServerName() + " 404 "
				+ client->getname() + " " + target_name
				+ " :Cannot send to channel");
		else
			channel->broadcast(full_message, client);
	}
	else
	{
		target_client = server->getClientByNick(target_name);
		if (!target_client)
			client->send_reply(":" + server->getServerName() + " 401 "
				+ client->getname() + " " + target_name
				+ " :No such nick/channel");
		else
			target_client->send_reply(full_message + "\r\n");
	}
}
