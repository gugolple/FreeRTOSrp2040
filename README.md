# RP2040 FreeRTOS

This is a basic project to test and learn FreeRTOS executed on an RP2040 due to it's simplicity and ubiquity.

## Goals

The following tasks are intended to be realized:

- Track the time over an LCD (HD44780 - 8 wire configuration)
- Keep the time (RTC type of operation/addon)
- RFID/NFC access and display over the screen

### Restrictions

The idea of the operation is to learn the microcontroller and not to produce a quick solution, for that the project will be realize only supported by standard libraries (like C11/C++11 and boost) and what is provided by FreeRTOS.

As far as possible it is desired to be based and utilize FOSS tools (not only free for the end user in an individual basis).

For the behaviours the way expected to be implemented is by following the manuals (that will be added to in the references section of this project).

# Tools

I will list all the tools used/planned to be used

- Editor: Vim/NeoVim
    - Autocompletion: [youcompleteme](https://github.com/ycm-core/YouCompleteMe)
- Documentation: [AsciiDoctor](https://asciidoctor.org/) (based on raspberry organization selection)
    - For diagrams it will be done in either: [Mermaid](https://mermaid.js.org/) [Graphviz](https://graphviz.org/) [Drawio](https://www.drawio.com/)
- Requierements: Based on [Zephyr](https://docs.zephyrproject.org/latest/safety/safety_requirements.html) [requirement repo](https://github.com/zephyrproject-rtos/reqmgmt) I chose (the same tool as them) [strictdoc](https://github.com/strictdoc-project/strictdoc)
- Building: Multilayer approach for maximun reusability and replicability
    - Podman: to manage the envrionment where it will be built (allowing to share the environment with the gitea actions)
    - Makefile: to automate all the specific command instructions to retrieve the final binary
    - CMake: the actual compilation infraestructure

# References

Based on [CORTEX_M0+\_RP2040](https://github.com/FreeRTOS/FreeRTOS-SMP-Demos/tree/main/FreeRTOS/Demo/CORTEX_M0%2B_RP2040)

Original [README](OG_README.md)
