CXX = g++
CXXFLAGS =-Wall -std=c++11 -g
LDFLAGS = -lpthread


src_a = Acceptor.cpp main_acceptor.cpp
src_p = Proposer.cpp main_proposer.cpp
src_l = Learner.cpp main_learner.cpp

obj_a = obj/Acceptor.o obj/main_acceptor.o
obj_p = obj/Proposer.o obj/main_proposer.o
obj_l = obj/Learner.o obj/main_learner.o


.PHONY: all test
all: obj Acceptor Proposer Learner

test: obj Acceptor Proposer Learner
	./Acceptor 12000 > acceptors_output.txt 2>&1 & ./Acceptor 12001 > acceptors_output.txt 2>&1 &\
       ./Acceptor 12002 > acceptors_output.txt 2>&1 & ./Learner 13000 > learner1_output.txt 2>&1 &\
       ./Learner 13001 > learner2_output.txt 2>&1 &
	sleep 1
	./Proposer 1 > proposer1_output.txt 2>&1 & ./Proposer 2 > proposer2_output.txt 2>&1 &\
       ./Proposer 3 > proposer3_output.txt 2>&1
	sleep 1
	killall Acceptor
	killall Learner
	cat learner1_output.txt

Acceptor: $(obj_a)
	$(CXX) -o $@ $^ $(LDFLAGS)

Proposer: $(obj_p)
	$(CXX) -o $@ $^ $(LDFLAGS)

Learner: $(obj_l)
	$(CXX) -o $@ $^ $(LDFLAGS)

obj: 
	mkdir obj	

# Create object files
$(obj_a): obj/%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(obj_p): obj/%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(obj_l): obj/%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

.PHONY: clean

clean:
	rm -rf obj Acceptor Proposer Learner

cleanall: clean
	rm -rf *_output.txt

test: all
	./Acceptor 12000 > acceptor1_output.txt 2>&1 &
	./Acceptor 12001 > acceptor2_output.txt 2>&1 &
	./Acceptor 12002 > acceptor3_output.txt 2>&1 &
	./Learner 13000 > learner1_output.txt 2>&1 &
	./Learner 13001 > learner2_output.txt 2>&1 &
	./Proposer > proposer_output.txt 2>&1 1
	killall Acceptor Learner
