import os
import sys
import shlex
import json
import random
import logging
import platform
import argparse
from pathlib import Path
from subprocess import call, run


def setup_logging(logging_level: str):

    level_map = {
        "info": logging.INFO,
        "debug": logging.DEBUG,
        "critical": logging.CRITICAL
    }

    if logging_level not in level_map:
        sys.stderr.write("Unrecognized logging level {}".format(logging_level))
        exit(1)

    logging.basicConfig(level=level_map[logging_level],
                        format='%(asctime)s: %(message)s',
                        datefmt='%H:%M:%S',
                        )

    formatter = logging.Formatter('%(asctime)s:%(levelname)s: %(message)s', datefmt='%H:%M:%S')
    file_handler = logging.FileHandler("run.trace", mode="w")
    file_handler.setLevel(level=level_map['debug'])
    file_handler.setFormatter(formatter)
    logging.getLogger().addHandler(file_handler)


def run_system_command(cmd: str,
                       shell: bool = False,
                       err_msg: str = None,
                       verbose: bool = True,
                       split: bool = True,
                       stdout=None,
                       stderr=None) -> int:
    """
    :param cmd: A string with the terminal command invoking an external program
    :param shell: Whether the command should be executed through the shell
    :param err_msg: Error message to print if execution fails
    :param verbose: Whether to print the command to the standard output stream
    :param split: Whether to split the tokens in the command string
    :param stdout: file pointer to redirect stdout to
    :param stderr: file pointer to redirect stderr to
    :return: Return code
    """
    if verbose:
        logging.debug("Current path: {}".format(Path.cwd()))
        logging.info("Run command: {}".format(cmd))
    if split and not shell:
        cmd = shlex.split(cmd)
    result = run(cmd, shell=shell, stdout=stdout, stderr=stderr)
    if err_msg and result.returncode:
        logging.critical(err_msg)
        exit(result.returncode)
    return result.returncode


def compile_game_engine():

    logging.info("Game engine not available!")
    logging.info("Compiling game engine..")

    current_path = Path.cwd()
    env_path     = current_path / "environment"

    if not os.path.exists(env_path):
        logging.critical("Couldn't find the engine sources - I expected them to be in ./environment")
        exit(1)

    os.chdir(env_path)
    run_system_command("make")
    run_system_command("cp halite ../halite")
    os.chdir(current_path)

    if not os.path.exists("halite"):
        logging.critical("Failed to produce executable environment!")
        exit(1)


def get_player_makefile():
    makefile = False
    if os.path.exists("makefile"):
        makefile = "makefile"
    elif os.path.exists("Makefile"):
        makefile = "Makefile"
    return makefile


def compile_player_bot():

    makefile = get_player_makefile()
    if makefile:
        logging.info("Compiling player sources..")
        run_system_command("make")


def clean_player_sources():

    makefile = get_player_makefile()
    if makefile:
        logging.info("Cleaning player data..")
        run_system_command("make clean")


def clean_logs():

    run_system_command("rm -f *.hlt", shell=True, verbose=False)
    run_system_command("rm -rf replays/", shell=True, verbose=False)
    run_system_command("rm -f *.log", shell=True, verbose=False)


def clean():

    clean_logs()
    clean_player_sources()


def check_environment():

    if not os.path.exists("halite"):
        compile_game_engine()

    compile_player_bot()

    for file in os.listdir("./visualizer/"):
        if file != "Visualizer.htm" and file.endswith(".htm"):
            run_system_command("rm ./visualizer/{}".format(file))

    clean_logs()
    run_system_command("mkdir ./replays")


def view_replay(browser, log):

    output_filename = log.replace(".hlt", ".htm")
    path_to_file    = os.path.join("visualizer", output_filename)

    if not os.path.exists(output_filename):

        # parse replay file
        with open(log, 'r') as f:
            replay_data = f.read()

        # parse template html
        with open(os.path.join("visualizer", "Visualizer.htm")) as f:
            html = f.read()

        html = html.replace("FILENAME", log)
        html = html.replace("REPLAY_DATA", replay_data)

        # dump replay html file
        with open(os.path.join("visualizer", output_filename), 'w') as f:
            f.write(html)

    with open("/dev/null") as null:
        call(browser + " " + path_to_file + " &", shell=True, stderr=null, stdout=null)


def play_round(player_cmd, game_round, browser):

    NUM_GAMES = 10
    TOP_GAMES = 8

    if not 1 <= game_round <= 5:
        logging.critical("Invalid round parameter {}, expected an integer value between 1 and 5".format(game_round))
        exit(1)

    logging.info("Playing round {}".format(game_round))

    configs = [
        (30, [25, 30, 40, 50], ["DBotv4"]),
        (10, [40], ["DBotv4"] * 3),
        (30, [25, 30, 40, 50], ["starkbot"]),
        (20, [40], ["starkbot", "DBotv4", "DBotv4"]),
        (10, [40], ["starkbot"] * 3)
    ]

    score, maps, bots = configs[game_round - 1]
    bots = ["\"./bots/{}_linux_x64\"".format(bot) for bot in bots]

    wins = 0
    map_configs = maps + [random.choice(maps) for _ in range(NUM_GAMES - len(maps))]
    engine_cmd  = ["./halite", "-q"]

    for game_idx, map_size in enumerate(map_configs):
        map_config = "-d \"{} {}\"".format(map_size, map_size)

        game_cmd = list(engine_cmd)
        game_cmd.append(map_config)
        game_cmd.append("\"{}\"".format(player_cmd))
        game_cmd += bots
        run_system_command(" ".join(game_cmd), verbose=False)
        game_log = None
        for file in os.listdir("./"):
            if file.endswith(".hlt"):
                game_log = file

        if not game_log:
            logging.critical("There was an error during the game, "
                             "no valid replay file was produced!")
        else:
            with open(game_log, "r") as f:
                result = json.loads(f.read())

                if "starkbot" in result["winner"] or "DBot" in result["winner"]:
                    game_type = "Defeat"
                else:
                    game_type = "Victory"
                    wins += 1

                logging.info("{} in {} steps!".format(game_type, result["num_frames"] - 1))

            if browser:
                view_replay(browser, game_log)

            run_system_command("mv {} ./replays".format(game_log), verbose=False)

    logging.critical("Round {} - wins {} / {} - done!".format(game_round, wins, NUM_GAMES))

    if wins >= TOP_GAMES:
        return score

    return (wins * 100 / TOP_GAMES) * score / 100


def main():

    parser = argparse.ArgumentParser(description='PA Bots Trainer', epilog="Happy Halite Hunting!")
    parser.add_argument('--cmd', required=True, help="Command line instruction to execute the bot. eg: ./MyBot")
    parser.add_argument('--round', type=int, default=0, help="Round index (1, 2, 3, 4, 5), default is 0 "
                                                             "which runs all of them")
    parser.add_argument('--clean', action="store_true", help="Remove logs/game results, call make clean")
    parser.add_argument('--visualizer', help="Input preffered browser name to display replays in browser; "
                                             "eg. \"firefox\", \"google-chrome-stable\"")
    parser.add_argument('--logging', default="info", help="(Optional) Define logging level for this script, "
                                                          "the options are: 'info', 'debug', 'critical'")
    args = parser.parse_args()

    if "Linux" not in platform.system():
        logging.critical("Warning: Script is designed to work for a 64bit version of Linux!")

    setup_logging(args.logging)
    check_environment()

    if args.round == 0:
        score = 0
        for game_round in range(1, 6):
            score += play_round(args.cmd, game_round, args.visualizer)
    else:
        score = play_round(args.cmd, args.round, args.visualizer)

    logging.critical("Scored {}/100.0 points".format(score))

    if args.clean:
        clean()


if __name__ == "__main__":
    main()
