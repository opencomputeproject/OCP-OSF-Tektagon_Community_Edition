/* shell command for testing */
#include <shell/shell.h>
#include <ctype.h>
#include <stdlib.h>
#include <device.h>
#include <drivers/timer/aspeed_timer.h>

#define TIMER_DEVICE_PREFIX		"TIMER"

static const char start_helper[] = "timer start <dev> -p <period> -t <type>";
static const char stop_helper[] = "timer stop <dev>";
static const char query_helper[] = "timer query <dev>";

struct timer_shell_data {
	const struct shell *shell;
	const struct device *dev;
	struct k_work work;
};

static struct timer_shell_data timer_shell_user_data;

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, TIMER_DEVICE_PREFIX);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}
SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

static void timer_shell_worker(struct k_work *work)
{
	struct timer_shell_data *data = CONTAINER_OF(work, struct timer_shell_data, work);
	const struct device *dev = data->dev;
	const struct shell *shell = data->shell;

	shell_print(shell, "%s expiration %d\n", dev->name, timer_aspeed_query(dev));
}

static void timer_shell_callback(void *user_data)
{
	struct timer_shell_data *data = (struct timer_shell_data *)user_data;

	k_work_submit(&data->work);
}

static int cmd_start(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev;
	struct getopt_state *state;
	struct aspeed_timer_user_config conf;
	int c, ret;

	dev = device_get_binding(argv[1]);
	if (!dev) {
		shell_error(shell, "TIMER: Device driver %s not found.", argv[1]);
		return -ENODEV;
	}

	while ((c = shell_getopt(shell, argc - 1, &argv[1], "hp:t:")) != -1) {
		state = shell_getopt_state_get(shell);
		switch (c) {
		case 'p':
			conf.millisec = strtoul(state->optarg, NULL, 0);
			break;
		case 't':
			conf.timer_type = strtoul(state->optarg, NULL, 0);
			break;
		case 'h':
			shell_help(shell);
			return SHELL_CMD_HELP_PRINTED;
		case '?':
			if ((state->optopt == 'p') || (state->optopt == 't')) {
				shell_print(shell, "Option -%c requires an argument.",
					    state->optopt);
			} else if (isprint(state->optopt)) {
				shell_print(shell, "Unknown option `-%c'.", state->optopt);
			} else {
				shell_print(shell, "Unknown option character `\\x%x'.",
					    state->optopt);
			}
			return 1;
		default:
			break;
		}
	}

	if (!timer_shell_user_data.work.handler) {
		k_work_init(&timer_shell_user_data.work, timer_shell_worker);
	}

	timer_shell_user_data.dev = dev;
	timer_shell_user_data.shell = shell;
	conf.callback = timer_shell_callback;
	conf.user_data = &timer_shell_user_data;
	shell_print(shell, "%s: period %d ms, type %d\n", dev->name, conf.millisec,
		    conf.timer_type);
	ret = timer_aspeed_start(dev, &conf);

	return ret;
}

static int cmd_stop(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev;
	int ret;

	dev = device_get_binding(argv[1]);
	if (!dev) {
		shell_error(shell, "TIMER: Device driver %s not found.", argv[1]);
		return -ENODEV;
	}

	shell_print(shell, "stop %s\n", dev->name);
	ret = timer_aspeed_stop(dev);

	return ret;
}

static int cmd_query(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev;
	int ret;

	dev = device_get_binding(argv[1]);
	if (!dev) {
		shell_error(shell, "TIMER: Device driver %s not found.", argv[1]);
		return -ENODEV;
	}

	ret = timer_aspeed_query(dev);
	shell_print(shell, "%s counter %d\n", dev->name, ret);

	return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_timer_cmds,
	SHELL_CMD(start, &dsub_device_name, start_helper, cmd_start),
	SHELL_CMD(stop, &dsub_device_name, stop_helper, cmd_stop),
	SHELL_CMD(query, &dsub_device_name, query_helper, cmd_query),
	SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(timer, &sub_timer_cmds, "TIMER commands", NULL);
