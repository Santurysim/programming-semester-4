# Сборка производится в каталоге build
# По умолчанию собирается программа build/cforth,
# но перед этим нужно собрать каталог build
all: build build/cforth

# Из каких модулей собирается программа
CFORTH_MODULES = main.cpp forth.cpp words.cpp
TEST_MODULES = test.cpp
# Общий вид правила:
# первая строка — фрагмент ориентированного графа зависимостей
# результат: зависимость-1 зависмость-2 ...
# далее, опционально, отступ (только Tab) и командs

# Для сборки программы build/cforth нужен каталог build (в конце)
# и список объектных файлов,
# который получается из списка исходных файлов в переменной CFORTH_MODULES,
# если к каждому из них добавить слева каталог "build/"
# и справа — расширение "%.o"
# Сборка будет выполняться компилятором C (переменная CC доступна по умолчанию)
# Вместо $^ подставляется перечень зависимостей из этого правила:
# build/main.c.o build/forth.c.o build/words.c.o
# Вместо $@ подставляется имя результата (build/cforth)

build/cforth: $(CFORTH_MODULES:%=build/%.o)
	$(CXX) $^ -o $@

# Общие (неизменяемые) настройки компилятора
# -MMD — сгенерировать файлы с описанием зависимостей (см. DEPS далее)
# -std=c99 -pedantic -Wall -Werror — приблизить поведение компилятора
#     к настройкам в дисплейных классах
# -I./include — искать файлы " … .h" в каталоге include
# Как и в языке C, Python и многих других,
# обратная косая черта (backslash, \ ) в самом конце строки
# обозначает перенос строки.
CFLAGS_COMMON = -MMD  -I./include \
	-std=c++98 -pedantic -Wall -Werror -Wextra -pedantic-errors \
	-Wpointer-arith -Waggregate-return \
	-Wmissing-declarations -Wcast-qual \
	-Wlong-long -Winline -Wredundant-decls \
	-Wcast-align -Wfloat-equal -D__STRICT_ANSI__
# Про предупреждения можно почитать в руководстве GCC:
# https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html

# Настройки компилятора по умолчанию (изменяемые)
# -g — сгенерировать информацию для отладки
# -O0 — отключить все оптимизации
# Можно вызвать с другими правилами: make CFLAGS="-Ofast -march=native" 
CFLAGS := -g -O0

CFLAGS_COVERAGE = -fprofile-arcs -ftest-coverage -fPIC

# Шаблонное правило:
#     для файлов вида build/(что-то).c.o
#     основная зависимость — src/(что-то).c
# $< — первая зависимость в списке (ещё есть зависимости из DEPS)
build/%.cpp.o: src/%.cpp
	$(CXX) $(CFLAGS_COMMON) $(CFLAGS) -c $< -o $@

build/test: src/test.cpp
	$(CXX) $(CFLAGS_COVERAGE) $(CFLAGS_COMMON) $(CFLAGS) $< -o $@ -lgcov

# Очистка — удаляем всё из каталога build
clean:
	rm -rf build/*
	rm -f ./*.gc*
	rm -f ./vgcore.*

# Мы подключаем все файлы с расширением .d, 
# которые найдём в каталоге build.
# Эти файлы создаст gcc с ключом -MMD при первой компиляции.
# В .d-файлах перечислен перечень файлов из проекта, которые включены в текущий
DEPS := $(shell find build -name *.d)

-include $(DEPS)

# Команда для сборки тестов
check: build build/test
	cd build && ./test

# Команда для оценки уровня покрытия кода тестами
.PHONY = coverage coverage_gcov
coverage: build/test check
	# cd build && ../bin/gcovr.sh -r .. --html --html-details -o coverage.html
	gcovr -r . --html --html-details -o build/coverage.html
	# kcov --include-path=./src build/coverage $<

coverage_gcov: build/test check
	gcov test

build:
	mkdir -p build

doc:
	doxygen doxygen.conf
