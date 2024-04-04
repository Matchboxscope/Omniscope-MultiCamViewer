

/*
brew install libserialport
brew install opencv
 clang++  -o app  ./ESP32/OmniscopeFW/CPP/decodeImageThread.cpp  -I/opt/homebrew/Cellar/libserialport/0.1.1/include -L/opt/homebrew/Cellar/libserialport/0.1.1/lib -lserialport -I/opt/homebrew/Cellar/opencv/4.8.1_5/include/opencv4 -L/opt/homebrew/Cellar/opencv/4.8.1_5/lib -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lserialport -std=c++11 -lpthread
 ./app -s /dev/cu.usbmodem117101

*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
// #include /opt/homebrew/Cellar/opencv/4.8.1_5/include/opencv4/opencv2/opencv.hpp
#include <opencv2/opencv.hpp>
#include <iostream>
#include <libserialport.h>
#include <cstring>
#include <vector>
#include <thread>

static const int B64index[256] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62, 63, 62, 62, 63, 52, 53, 54, 55,
                                  56, 57, 58, 59, 60, 61, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6,
                                  7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0,
                                  0, 0, 0, 63, 0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
                                  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};

// Flag to control the reading loop in each thread
std::atomic<bool> keepRunning(true);
//std::mutex serialPortMutex;

std::string b64decode(const void *data, const size_t len)
{
    unsigned char *p = (unsigned char *)data;
    int pad = len > 0 && (len % 4 || p[len - 1] == '=');
    const size_t L = ((len + 3) / 4 - pad) * 4;
    std::string str(L / 4 * 3 + pad, '\0');

    for (size_t i = 0, j = 0; i < L; i += 4)
    {
        int n = B64index[p[i]] << 18 | B64index[p[i + 1]] << 12 | B64index[p[i + 2]] << 6 | B64index[p[i + 3]];
        str[j++] = n >> 16;
        str[j++] = n >> 8 & 0xFF;
        str[j++] = n & 0xFF;
    }
    if (pad)
    {
        int n = B64index[p[L]] << 18 | B64index[p[L + 1]] << 12;
        str[str.size() - 1] = n >> 16;

        if (len > L + 2 && p[L + 2] != '=')
        {
            n |= B64index[p[L + 2]] << 6;
            str.push_back(n >> 8 & 0xFF);
        }
    }
    return str;
}

int frameCount = 0;

void handleSerialPort(const std::string &portName)
{
    struct sp_port *port;
    // Open the port
    if (sp_get_port_by_name(portName.c_str(), &port) != SP_OK)
    {
        std::cerr << "Error finding port " << portName << std::endl;
        return;
    }

    if (sp_open(port, SP_MODE_READ_WRITE) != SP_OK)
    {
        std::cerr << "Error opening port " << portName << std::endl;
        sp_free_port(port);
        return;
    }

    // Set serial port parameters
    sp_set_baudrate(port, 2000000);
    sp_set_bits(port, 8);
    sp_set_parity(port, SP_PARITY_NONE);
    sp_set_stopbits(port, 1);
    sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE);

    //std::cout << "Reading data..." << std::endl;
    while (keepRunning)
    {

        // Lock the mutex before accessing the serial port
        // std::lock_guard<std::mutex> lock(serialPortMutex);

        // send string "c\n" to request the camera to send an image
        // Read data from serial port
        const char *command = "c\n";
        // std::cout << "Sending command: " << command << std::endl;
        sp_blocking_write(port, command, strlen(command), 50);
        sp_drain(port); // Waits until all data has been transmitted

        // Buffer for reading data
        std::string buffer;
        char tempBuf[1024 * 16];
        int bytes_read;

        int requestLoopCounter = 0;
        while (true)
        {
            if (requestLoopCounter > 2)
            {
                std::cerr << "Error: Timeout while waiting for frame" << std::endl;
                break;
            }
            bytes_read = sp_blocking_read(port, tempBuf, sizeof(tempBuf), 100);
            tempBuf[bytes_read] = '\0';
            buffer += std::string(tempBuf);
            //std::cout << "Bytes read: " << bytes_read << "from port: " << portName << std::endl;

            // if no bytes are read, increase the counter
            if (bytes_read <= 0)
            {
                requestLoopCounter++;
                continue;
            }
            else
            {
                requestLoopCounter = 0;
            }
            // Check for line break indicating end of frame
            size_t lineBreakPos = buffer.find('\n');
            if (lineBreakPos != std::string::npos)
            {
                // Extract base64-encoded frame and decode
                std::string encodedFrame = buffer.substr(0, lineBreakPos);
                std::string decodedFrame = b64decode(encodedFrame.c_str(), encodedFrame.size());
                //std::cout << "Decoded frame size: " << buffer << std::endl;
                // Process the decoded frame
                std::vector<uchar> data(decodedFrame.begin(), decodedFrame.end());
                cv::Mat image = cv::imdecode(cv::Mat(data), cv::IMREAD_COLOR);

                if (!image.empty())
                {
                    // Display or further process the image
                    // cv::imshow("Frame", image);
                    // cv::waitKey(1); // Update display window

                    // Optionally, save the frame to a file
                    if (0)
                    {
                        std::string filename = "frame_" + std::to_string(frameCount++) + ".jpg";
                        cv::imwrite(filename, image);
                    }
                    else if (0)
                    {
                        cv::imshow(portName, image);
                        cv::waitKey(1); // Update display window
                    }
                    else
                    {

                        // write image to file in the same directory using portname 
                        // split filename at "/" and take the last part
                        std::string portNameLastPart = portName.substr(portName.find_last_of("/")+1);
                        std::string filename = "frame_" + portNameLastPart + ".jpg";
                        std::cout << "Writing to file: " << filename << std::endl;
                        cv::imwrite(filename, image);
                    }
                }


                buffer.erase(0, lineBreakPos + 1);
                // reset tempBuf
                memset(tempBuf, 0, sizeof(tempBuf));
                break;
            }
        }
        // Sleep for a short duration to give other threads a chance
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // Close the port
    sp_close(port);
    sp_free_port(port);
}

int main(int argc, char *argv[])
{
    std::vector<std::string> serialPorts;
    std::vector<std::thread> threads;

    // Skip the first argument (program name) and collect serial port names
    for (int i = 2; i < argc; ++i)
    {
        serialPorts.push_back(argv[i]);
        std::cout << "Serial port: " << argv[i] << std::endl;
    }

    // Check if at least one serial port was provided
    if (serialPorts.empty())
    {
        std::cerr << "Usage: " << argv[0] << " <serial_port1> <serial_port2> ..." << std::endl;
        return 1;
    }

    // Launch a thread for each serial port
    for (const auto &portName : serialPorts)
    {
        threads.emplace_back(std::thread(handleSerialPort, portName));
    }

    // Wait for all threads to complete
    for (auto &thread : threads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    std::cout << "All threads completed." << std::endl;
    return 0;
}