#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdint>

void error(const char *msg);
uint16_t mov_parse(const std::string& operands);
uint16_t reg_num_parse(const std::string& reg_name);
std::string ins2str(uint16_t pc, uint16_t iword);

int main(int argc, char *argv[]) {
	if (argc != 3)
		error("Usage: eepasm infile outfile");

	std::ifstream infile {argv[1]};
	if (!infile)
		error("can't open input file");
	std::ofstream outfile {argv[2]};
	if (!outfile)
		error("can't open output file");

	std::string opcode;
	std::string operands;
	uint16_t pc = 0, iword;
	while (infile >> opcode && getline(infile, operands)) {
		operands = operands.substr(1); // strip leading space
		if (opcode == "MOV" || opcode == "mov") {
			iword = mov_parse(operands);
		} else if (opcode == "ADD" || opcode == "add") {
			iword = 0;
		} else if (opcode == "SUB" || opcode == "sub") {
			iword = 0;
		} else if (opcode == "ADC") {
			iword = 0;
		} else if (opcode == "SDC") {
			iword = 0;
		} else if (opcode == "AND") {
			iword = 0;
		} else if (opcode == "CMP") {
			iword = 0;
		} else if (opcode == "LSL") {
			iword = 0;
		} else if (opcode == "LSR") {
			iword = 0;
		} else if (opcode == "ASR") {
			iword = 0;
		} else if (opcode == "XSR") {
			iword = 0;
		} else if (opcode == "LDR") {
			iword = 0;
		} else if (opcode == "STR") {
			iword = 0;
		} else if (opcode == "JMP") {
			iword = 0;
		} else if (opcode == "NOOP") {
			iword = 0;
		} else if (opcode == "JEQ") {
			iword = 0;
		} else if (opcode == "JNE") {
			iword = 0;
		} else if (opcode == "JCS") {
			iword = 0;
		} else if (opcode == "JCC") {
			iword = 0;
		} else if (opcode == "JMI") {
			iword = 0;
		} else if (opcode == "JPL") {
			iword = 0;
		} else if (opcode == "JGE") {
			iword = 0;
		} else if (opcode == "JLT") {
			iword = 0;
		} else if (opcode == "JGT") {
			iword = 0;
		} else if (opcode == "JLE") {
			iword = 0;
		} else if (opcode == "JHI") {
			iword = 0;
		} else if (opcode == "JLS") {
			iword = 0;
		} else if (opcode == "JSR") {
			iword = 0;
		} else if (opcode == "RET") {
			iword = 0;
		} else if (opcode == "EXT") {
			iword = 0;
		} else {
			error("Unknown instruction");
		}
		outfile << ins2str(pc++, iword) << "\n";
	}
}

void error(const char *msg) {
	std::cerr << "Error: " << msg << std::endl;
	std::exit(EXIT_FAILURE);
}

std::string ins2str(uint16_t pc, uint16_t iword) {
	std::ostringstream out;
	if (pc <= 0xf)
		out << "0x0" << std::hex << pc << " ";
	else
		out << "0x" << std::hex << pc << " ";

	if (iword <= 0xf)
		out << "0x000" << std::hex << iword;
	else if (iword <= 0xff)
		out << "0x00" << std::hex << iword;
	else if (iword <= 0xfff)
		out << "0x0" << std::hex << iword;
	else
		out << "0x" << std::hex << iword;

	return out.str();
}

uint16_t reg_num_parse(const std::string& reg_name) {
	return reg_name[1] - '0';
}

// parses the given immediate operand with the leading #
// returns the number after the # anded with bitmask
uint16_t imm_parse(const std::string& imm_op, uint16_t bitmask) {
	uint16_t num;
	if (imm_op.substr(0,3) == "#0x")
		num = std::stoi(imm_op.substr(3), nullptr, 16); // base 16
	else if (imm_op.substr(0,3) == "#0b")
		num = std::stoi(imm_op.substr(3), nullptr, 2); // base 2
	else
		num = std::stoi(imm_op.substr(1)); // base 10
	return num & bitmask;
}

uint16_t mov_parse(const std::string& operands) {
	std::istringstream op_stream {operands};
	std::string operand;
	uint16_t iword = 0x0;
	
	// first operand (has to be register)
	op_stream >> operand;
	operand.replace(2, 3, ""); // remove trailing comma
	iword += reg_num_parse(operand) << 9;

	// second operand (can be register or immediate value)
	op_stream >> operand;
	if (operand[0] == '#') {
	    iword += 1 << 8; // set iword[8] bit
	    iword += imm_parse(operand, 0xff); // mask for 8 bit immediate
	} else {
	    iword += reg_num_parse(operand) << 5;
	}
	return iword;
}
