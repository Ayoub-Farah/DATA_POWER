import sys
import time

def progress_bar(iteration, total, prefix='', suffix='', length=50, fill='█'):
    """
    Call in a loop to create terminal progress bar
    @params:
        iteration   - Required  : current iteration (Int)
        total       - Required  : total iterations (Int)
        prefix      - Optional  : prefix string (Str)
        suffix      - Optional  : suffix string (Str)
        length      - Optional  : character length of bar (Int)
        fill        - Optional  : bar fill character (Str)
    """
    percent = ("{0:.1f}").format(100 * (iteration / float(total)))
    filled_length = int(length * iteration // total)
    bar = fill * filled_length + '-' * (length - filled_length)
    sys.stdout.write('\r%s |%s| %s%% %s' % (prefix, bar, percent, suffix)),
    sys.stdout.flush()
    # Print New Line on Complete
    if iteration == total:
        print()

# # Example Usage
# total_iterations = 100
# for i in range(total_iterations):
#     time.sleep(0.1)  # Simulate a long-running process
#     progress_bar(i + 1, total_iterations, prefix='Progress:', suffix='Complete', length=40)

# print("Task completed successfully!")
