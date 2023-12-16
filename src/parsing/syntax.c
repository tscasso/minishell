/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   syntax.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tscasso <tscasso@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/11/21 16:53:50 by tscasso           #+#    #+#             */
/*   Updated: 2023/12/14 19:20:25 by tscasso          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

void	add_token_to_list(t_node **head, t_node *new_token)
{
	t_node	*current;

	if (*head == NULL)
	{
		*head = new_token;
		new_token->previous = NULL;
	}
	else
	{
		current = *head;
		while (current->next != NULL)
		{
			current = current->next;
		}
		current->next = new_token;
		new_token->previous = current;
	}
}

t_node	*create_new_token(char *value, t_lexer *lexer)
{
	t_node	*new_token;

	new_token = malloc(sizeof(t_node));
	if (new_token == NULL)
	{
		fprintf(stderr, "Erreur d'allocation mémoire pour le token\n");
        exit(EXIT_FAILURE);
	}
	new_token->value = value;
	new_token->next = NULL;
	new_token->previous = NULL;
	if (lexer->quotes == IN_DOUBLE_QUOTES)
	{
		new_token->quote_state = IN_DOUBLE_QUOTES;
		lexer->quotes = NO_QUOTES;
	}
	else if (lexer->quotes == IN_SINGLE_QUOTES)
	{
		new_token->quote_state = IN_SINGLE_QUOTES;
		lexer->quotes = NO_QUOTES;
	}
	else
		new_token->quote_state = NO_QUOTES;
	return (new_token);
}

void	parse_token(t_node *token, t_lexer *lex)
{
	if (token->quote_state == NO_QUOTES && !lex->previous_token)
		token->type = COMMAND;
	else if (token->quote_state == NO_QUOTES && lex->previous_token->type == PIPE)
		token->type = COMMAND;
	else if (token->value[0] == '-' && token->quote_state == NO_QUOTES && lex->previous_token->type == COMMAND)
		token->type = COMMAND;
	else if ((token->quote_state == IN_DOUBLE_QUOTES || token->quote_state == NO_QUOTES) && ft_strcmp(&(token->value[0]), "$") == 0)
		token->type = ENV_VAR;
	else if((token->quote_state == IN_DOUBLE_QUOTES || token->quote_state == IN_SINGLE_QUOTES) && lex->previous_token != NULL)
		token->type = ARGUMENT;
	else if (strcmp(&(token->value[0]), "|") == 0)
		token->type = PIPE;
	else if (strcmp(&(token->value[0]), "&") == 0)
		token->type = AMPER;
	else if (strcmp(&(token->value[0]), ">") == 0)
		token->type = REDIRECTION;
	else if (strcmp(&(token->value[0]), "<") == 0)
		token->type = LEFT_REDIRECT;
	else if (strcmp(token->value, ">>") == 0)
		token->type = DOUBLE_REDIRECT;
	else if (ft_strcmp(token->value, "<<") == 0)
		token->type = HEREDOC;
	else
		token->type = ARGUMENT;
	lex->previous_token = token;
}

void	add_token(t_lexer *lexer, char *token_value)
{
	t_node	*new_token;

	new_token = create_new_token(token_value, lexer);
	parse_token(new_token, lexer);
	add_token_to_list(&(lexer->token_list->head), new_token);
}
