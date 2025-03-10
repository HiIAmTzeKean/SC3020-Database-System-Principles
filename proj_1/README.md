## Source Code URL
[Source Code](https://entuedu-my.sharepoint.com/:f:/g/personal/tng042_e_ntu_edu_sg/EpFYMbNqYd5Dk4F2YJ9evGYBnjDEhLRil594hJ2cFNEFxA)

## Video URL
[Video](https://entuedu-my.sharepoint.com/:f:/g/personal/tng042_e_ntu_edu_sg/EuJGpmUvL5ZMhIazgiby5BoBnwKDjwN2DexSmlKGB9Wplw)

## Introduction
In this project, our group designs and implements the storage and indexing components of a database management system. This project has been implemented in C++.

## Access and Installation Guide
1. Download the source code folder from the provided URL and unzip the downloaded `.zip` file and open a command prompt or terminal at the unzip location.
2. This project requires a C++ compiler. Mingw, clang, or gcc are recommended.
3. Compile the provided source files into an executable by adapting the command below for your compiler. We use `-O3` to have a more accurate sense of how long it will take as our B+ tree implementation has plenty of checks and assertions that slow down execution.

For Windows
```sh
g++ -std=c++17 -g -Wall -O3 main.cpp bp_tree.cpp node.cpp task.cpp storage/data_block.cpp storage/serialize.cpp storage/storage.cpp -o main.exe -w
```

For Mac/Linux
```sh
g++ -std=c++17 -g -Wall -O3 *.cpp storage/*.cpp -o main
```

4. Run the executable file. The expected arguments are the value of `N` and the text file with the information to process. Examples of common invocations are provided below.

Windows
```sh
.\main.exe 9 games.txt
```

Mac/Linux
```sh
./main 9 games.txt
```