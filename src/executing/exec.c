/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exec.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tscasso <tscasso@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/12/14 19:24:40 by tscasso           #+#    #+#             */
/*   Updated: 2023/12/16 19:18:06 by tscasso          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/minishell.h"

// Si builtin, on bifurque avant execve
// Si on est dans une commande simple ou la deuxieme commande du pipe (cad dans le parent)
// on fork pour eviter d'exit du minishell
// On remet l'env sous forme de tableau et on envoie a execve
int		run_simple_command(char **args, t_env *env, int mode, t_parser *data)
{
	char	**env_tab;
	int		status;
	pid_t	pid;

	if (is_builtin(args[0]))
		return (exec_builtin(args, env));
	else
	{
		if (mode == 1)
			pid = fork();
		else
			pid = 1;
		if (pid == 0 || mode == 0)
		{
			env_tab = env_to_str(env);
			if (execve(args[0], args, env_tab) == -1)
			{
				perror("execve");
				return(1);
			}
			else if (mode == 1)
			{
				free_command(data->command_list);
				_exit(0);
			}
		}
		else if (pid > 0 && mode == 1)
		{
			waitpid(-1, &status, 0);
			if (WEXITSTATUS(status) == 1)
				return (1);
		}
	}
	return (0);
}

int		setup_pipe_parent(t_parser *data, t_env *env, int out, t_exec *exec)
{
	waitpid(-1, &(exec->status), 0);
	if (WEXITSTATUS(exec->status) == 1)
		printf("error in the first command");
	close(exec->fd[1]);
	dup2(exec->fd[0], STDIN_FILENO);
	close(exec->fd[0]);
	dup2(out, STDOUT_FILENO);
	close(out);
	if ((exec->cmd2 = build_exec_line(data->command_list->head->next, env)) == NULL)
		return (1);
	if (run_simple_command(exec->cmd2, env, 1, data))
		return (1);
	return (0);
}

int		setup_pipe_child(t_parser *data, t_env *env, t_exec *exec)
{
	close(exec->fd[0]);
	dup2(exec->fd[1], STDOUT_FILENO);
	close(exec->fd[1]);
	if ((exec->cmd1 = build_exec_line(data->command_list->head, env)) == NULL)
		return (1);
	if (run_simple_command(exec->cmd1, env, 0, data))
		return (1);
	else
	{
		free_command(data->command_list);
		my_free(exec->cmd1);
		_exit(0);
	}
	return (0);
}

int		setup_pipe(t_parser *data, t_env *env, int out, t_exec *exec)
{
	if (pipe(exec->fd) == -1 || (exec->pid = fork()) == -1)
	{
		perror("fatal error");
		return (1);
	}
	if (exec->pid == 0)
		return (setup_pipe_child(data, env, exec));
	else
		return (setup_pipe_parent(data, env, out, exec));
}

void	execute_simple_cmd(t_parser *data, t_env *env, t_exec *exec)
{
	if (ft_strcmp(exec->full_cmd->head->value, "exit") == 0)
	{
		freelist(env);
		my_free(exec->cmd1);
		free_command(data->command_list);
		free(data);
		exit(0);
	}
	if ((exec->cmd = build_exec_line(data->command_list->head, env)) == NULL)
		return;
	if (run_simple_command(exec->cmd, env, 1, data))
	{
		my_free(exec->cmd1);
		return;
	}
}

void	execute_command(t_parser *data, t_env *env)
{
	t_exec	*exec;

	exec = malloc(sizeof(t_exec));
	init_exec(exec, data);
	while (exec->full_cmd)
	{
		if (check_for_pipe(exec->full_cmd))
		{
			if (setup_pipe(data, env, exec->original_out_fd, exec))
				break;
			exec->full_cmd = exec->full_cmd->next->next;
		}
		else
		{
			execute_simple_cmd(data, env, exec);
			exec->full_cmd = exec->full_cmd->next;
		}
	}
	reset_pipes(exec);
	free(exec);
}

// Configure le pipe et le fork pour creer un processus enfant
// Cree le tableau de string pour envoie a execve ou aux builtins
// Envoie du tableau pour l'execution, free et exit du processus enfant de la premiere commande
// Pendant ce temps le process parent attend la fin de la premiere commande
// Puis execute la deuxieme commande
// int	setup_pipe(t_parser *data, t_env *env, int out, t_exec *exec)
// {
// 	if (pipe(exec->fd) == -1 || ((exec->pid = fork()) == -1))
// 	{
// 		perror("error fatal");
// 		return(1);
// 	}
// 	if (exec->pid == 0)
// 	{
// 		close(exec->fd[0]); // On ferme l'entree du pipe qui n'est pas utilise ici
// 		dup2(exec->fd[1], STDOUT_FILENO); // On duplique la sortie du pipe sur la sortie standard 
// 		close(exec->fd[1]); // On ferme la sortie du pipe
// 		if ((exec->cmd1 = build_exec_line(data->command_list->head, env)) == NULL)
// 			return (1);
// 		if (run_simple_command(exec->cmd1, env, 0, data))
// 			return(1);
// 		else
// 		{
// 			free_command(data->command_list); // free de toutes les allocs dans le process enfant
// 			_exit(0);
// 		}
// 	}
// 	else
// 	{
// 		waitpid(-1, &(exec->status), 0); // on attend que le process enfant se termine
// 		if (WEXITSTATUS(exec->status) == 1)
// 			printf("erreur premiere commande\n");
// 		close(exec->fd[1]); // on ferme la sortie du pipe qui n'est pas utilise ici
// 		dup2(exec->fd[0], STDIN_FILENO); // on duplique l'entree du pipe sur la sortie standard
// 		close(exec->fd[0]); // on ferme l'entree du pipe
// 		dup2(out, STDOUT_FILENO); // on s'assure que la sortie de la commande est bien revenue sur la sortie standard
// 		close(out);
// 		if ((exec->cmd2 = build_exec_line(data->command_list->head->next, env)) == NULL)
// 			return (1);
// 		if (run_simple_command(exec->cmd2, env, 1, data))
// 			return(1);
// 	}
// 	return (0);
// }

// Enregistre le fd de la sortie et de l'entree standard pour reinitialiser apres la commande
// Parcours la liste de sous liste et verifie s'il y a un pipe
// Distribue selon s'il y a un pipe entre commande simple et commande complexe
// void execute_command(t_parser *data, t_env *env)
// {
// 	t_list	*full_cmd;
// 	char	**cmd1;
// 	int		original_in_fd;
// 	int		original_out_fd;

// 	original_in_fd = dup(STDIN_FILENO);
// 	original_out_fd = dup(STDOUT_FILENO);
// 	full_cmd = data->command_list->head;
// 	while (full_cmd)
// 	{
// 		if (check_for_pipe(full_cmd))
// 		{
// 			if (setup_pipe(data, env, original_out_fd))
// 				break;
// 			full_cmd = full_cmd->next->next;
// 		}
// 		else
// 		{
// 			if ((cmd1 = build_exec_line(data->command_list->head, env)) == NULL)
// 				break;
// 			if (run_simple_command(cmd1, env, 1, data))
// 				break;
// 			full_cmd = full_cmd->next;
// 		}
// 	}
// 	dup2(original_in_fd, STDIN_FILENO);
// 	dup2(original_out_fd, STDOUT_FILENO);
// 	close(original_in_fd);
// 	close(original_out_fd);
// }
