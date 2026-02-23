/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   quit.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: liliu <liliu@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/03 16:22:08 by liliu             #+#    #+#             */
/*   Updated: 2026/02/04 16:17:19 by liliu            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/channel.hpp"
#include "../include/client.hpp"
#include "../include/commands.hpp"
#include "../include/server.hpp"

void	handle_quit(Server *server, Client *client,
		std::vector<std::string> &args)
{
	Channel	*channel;

	std::string quit_msg = (args.empty()) ? "Client quit" : args[0];
	std::string quit_notification = ":" + client->getname() + "!"
		+ client->getUsername() + "@" + client->getip() + " QUIT :" + quit_msg;
	const std::map<std::string, Channel *> &channels = server->getChannels();
	for (std::map<std::string,
		Channel *>::const_iterator it = channels.begin(); it != channels.end(); ++it)
	{
		channel = it->second;
		if (channel->isClientInChannel(client))
		{
			channel->broadcast(quit_notification, client);
			channel->removeClient(client);
		}
	}
	client->send_reply("ERROR :Closing connection: " + quit_msg);
}