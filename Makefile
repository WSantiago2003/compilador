# Compilador e flags
CC = gcc
CFLAGS = -Wall

# Nome do executável do seu compilador
TARGET = compilador.exe

# Arquivos fonte do seu projeto
SRCS = main.c lexer.c parser.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

# O novo "make run" automatizado para Windows CMD
run: $(TARGET)
	$(TARGET) teste.txt > saida.c
	$(CC) saida.c -o programa_final.exe
	programa_final.exe

clean:
	del $(TARGET) saida.c programa_final.exe