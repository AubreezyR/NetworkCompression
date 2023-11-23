# Networks Final Project

## Table of Contents
1. [Description](#description)
2. [Instructions](#instructions)
3. [Incomplete Features](#incomplete-features)
7. [Contributers](#contributers)

## Description
This project is an example of End-to-End Detection of Network Compression. It is a standalone application as well as a non standalone application that uses a third party JSON parser as well as an inputed config file.
  # Standalone:
  In this version of the application it will detect compression upon the time different between two SYN packet with a UDP packet train inbetween them.
  # Non-Standalone:
  This application will detect compression upon the time difference between the start/end of a low entropy packet and start/end of a high entropy packet train. The results are then sent to the client which says wether or not compression was detected 


## Instructions
In order to run this program:
1. Clone the repo from github or include the following files in the same folder:
    -cJSON.h
    -cSJON.c
    -standalone.c
    -standalone_server.c
    -compdetect_client.c
    -comdetect_server.c
    -a config.json file
3. Once you have the correct file structure run the command "gcc -o standalone_server standalone_server.c cJSON.c -lm" to compile the program
4. do the same with standalone_server or compdetect_client & compdetect_server.c (depending on which you plan to run)
5. In order to run the program you must write "./<application name> <port number> for server applications and ./<application name> <config file> for client side applications (adding a sudo before for the stand alone version) Ex. sudo ./standalone config.json


## Incomplete Features
While most of the program fulfills the required features, I did encounter challenges in one particular area that ultimately remained unfulfilled. I could not receive RST packets in the program. I worked on this for a considerable amount of time and confirmed that the packets were being sent correctly. However, the reception of RST packets proved elusive. As a temporary workaround, the timers initiate when I send the two SYN packets, and the final compression result is obtained by calculating the difference between the two.
In the future my plan to capture the RST packets would use the multithreading library threads.h inorder to both send he SYN and UDP packets on one thread and listen for the RST packets on another thread, starting and stopping the timer at the apropriate places and then calculating the time to see if compression occured or not.
## Contributers
Aubrey Robinson
