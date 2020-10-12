#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
parse local log file or message via networking url,  plot the progress dialog
The percentage format is defaulted as  r" ([0-9,.]+) percent", but can be provided as the third argument.

For local file, using QTimer to poll  the log file change, pygtail module from PyPI can be a choise
For network url liek websocket, using Qt event loop  (not yet implemented)
This script has been tested to work with PyQt5 on Windows and PySide2 on Linux
"""
USAGE = "pppMonitorProgress.py log_file  [window_title]  [percentage_regex_pattern]"

import sys
import os.path
import time
import re
import subprocess

try:
    from qtpy import QtGui, QtCore, QtWidgets
    from qtpy.QtCore import QTimer
    from qtpy.QtWidgets import (
        QApplication,
        QMessageBox,
        QMainWindow,
        QVBoxLayout,
        QHBoxLayout,
        QPushButton,
        QLabel,
        QWidget,
        QProgressBar,
        QAction,
    )
except ImportError:
    print("Qt for python (pyside2) or qtpy is not installed, so the external graphic progress monitor is not available")
    sys.exit()


class ProgressMonitor(QMainWindow):
    #
    def __init__(self, log_file, windows_title, percentage_pattern):
        super(ProgressMonitor, self).__init__()

        self.setGeometry(50, 50, 500, 100)
        self.setWindowTitle(windows_title)
        self.setWindowIcon(QtGui.QIcon("pythonlogo.png"))

        closeAction = QAction("close", self)
        closeAction.setShortcut("Ctrl+Q")
        closeAction.setStatusTip("close this window")
        closeAction.triggered.connect(self.close_application)

        self.polling_interval_in_seconds = 2
        self.elapsed_time = 0.0
        self.elapsed_time_label = QLabel()
        self.elapsed_time_label.setText("elapsed time: 0 second")

        self.progress_bar = QProgressBar(self)
        # self.progress_bar.resize(400, 25)
        # self.progress_bar.setGeometry(200, 50, 250, 20)

        self.progress = 0.0  #  100 as completion
        self.last_line_number = 0
        self.percentage_pattern = percentage_pattern

        killbtn = QPushButton("Interrupt", self)
        killbtn.clicked.connect(self.interrupt_application)

        btn = QPushButton("Quit", self)
        btn.clicked.connect(self.close_application)
        btn.resize(btn.minimumSizeHint())

        hbox = QHBoxLayout()
        hbox.addWidget(killbtn)
        hbox.addStretch()
        hbox.addWidget(btn)

        vbox = QVBoxLayout()
        vbox.addWidget(self.elapsed_time_label)
        vbox.addWidget(self.progress_bar)
        vbox.addLayout(hbox)

        widget = QWidget(self)
        widget.setLayout(vbox)
        self.setCentralWidget(widget)
        self.setup_monitor(log_file)

    def setup_monitor(self, log_file):
        # if string type, and os.path.exists()
        self.log_file = log_file

        self.monitor_timer = QTimer(self)
        self.monitor_timer.start(self.polling_interval_in_seconds * 1000)
        self.monitor_timer.timeout.connect(self.monitor)

    def get_lines(self):
        new_lines = []
        with open(self.log_file, "r+") as f:
            # fcntl.flock(f, fcntl.LOCK_EX)  # make no difference
            all_lines = f.readlines()
            # fcntl.flock(f, fcntl.LOCK_UN)
            new_line_number = len(all_lines)
            if new_line_number > self.last_line_number:
                new_lines = all_lines[self.last_line_number :]
                self.last_line_number = new_line_number
            # print(all_lines, new_lines)
        return new_lines

    def get_lines_pipe(self):
        # also work working
        cmdline = ["tail", self.log_file]
        p = subprocess.Popen(cmdline, stdout=subprocess.PIPE)
        out = p.communicate()[0]
        new_lines = out.decode("ascii").split("\n")
        print(new_lines[-1])
        return new_lines

    def monitor(self):
        # new_lines = self.get_lines_pipe()
        new_lines = self.get_lines()
        self.progress = self.parse(new_lines)

        if self.progress >= 100:
            self.progress = 100
            sys.exit()  # auto close
        self.elapsed_time += self.polling_interval_in_seconds
        self.elapsed_time_label.setText(
            "elapsed time: " + str(self.elapsed_time) + " seconds"
        )
        self.progress_bar.setValue(self.progress)

    def parse(self, new_lines):
        p = self.progress
        for l in reversed(new_lines):
            result = re.search(self.percentage_pattern, l)
            if result:
                match_float = result.group(1)  # not a list
                if len(match_float) > 1:
                    p = float(match_float)
                    # print(l, p)
                    if p >= self.progress:
                        return p
        return self.progress

    def interrupt_application(self):
        sys.exit()

    def close_application(self):
        sys.exit()
        # choice = QMessageBox.question(
        #     self,
        #     "close!",
        #     "close this progress monitor, without interrupt the target process?",
        #     QMessageBox.Yes | QMessageBox.No,
        # )
        # if choice == QMessageBox.Yes:
        #     sys.exit()
        # else:
        #     pass


def generate_log(log_file):
    # close file after each writing, for testing
    for i in range(10):
        if i == 0:
            logf = open(log_file, "w")
        else:
            logf = open(log_file, "a")
        p = (i + 1) * 10.0
        logf.write(f"completed {p} percent\n")  # do not forget newline
        print(f"completed {p} percent, print to stdout")
        time.sleep(1)
        logf.close()


def generate_log_continuously(log_file):
    # working with and without fcntl locking
    using_lock = False
    logf = open(log_file, "w")  # must close so the other thread can read
    for i in range(10):
        # Allow only one process to hold an exclusive lock for a given file at a given time
        if using_lock:
            try:
                import fcntl  # POSIX only

                fcntl.flock(logf, fcntl.LOCK_EX)
            except IOError:
                print("failed to lock file")
        p = (i + 1) * 10.0
        logf.write(f"complated {p} percent\n")  # do not forget newline
        logf.flush()  # do not forget this, otherwise file is empty
        if using_lock:
            try:
                import fcntl  # POSIX only

                fcntl.flock(logf, fcntl.LOCK_UN)
            except IOError:
                print("failed to unlock file")
        print(f"completed {p} percent, print to stdout")
        time.sleep(1)


if __name__ == "__main__":
    windows_title = "progress"
    percentage_pattern = r" ([0-9,.]+) percent"
    if len(sys.argv) < 2:
        print(USAGE)
        # sys.exit(1)
        # set None or test_log_file for testing purpose, comment out after testing
        from tempfile import gettempdir
        import threading

        windows_title = "It is a test demo if no logfile argument is provided"
        log_file = gettempdir() + os.path.sep + "test_progress.log"
        x = threading.Thread(target=generate_log_continuously, args=(log_file,))
        x.start()
    else:
        log_file = sys.argv[1]
        if len(sys.argv) > 2:
            windows_title = sys.argv[2]
        if len(sys.argv) > 3:
            percentage_pattern = sys.argv[3]

    app = QApplication(sys.argv)
    GUI = ProgressMonitor(log_file, windows_title, percentage_pattern)
    GUI.show()
    sys.exit(app.exec_())
