#ifndef EEPASM_H
#define EEPASM_H

using oplist_t = std::vector<std::unordered_map<std::string, std::string>>;
using insmap_t = std::unordered_map<std::string, std::pair<std::vector<oplist_t>, uint16_t>>;
using labelmap_t = std::unordered_map<std::string, int>;
using tokvec_t = std::vector<std::vector<std::string>>;

constexpr char def_insfile[] = "inslist.eepc";
constexpr char def_outfile[] = "out.ram";
constexpr int regsize = 3;
constexpr int offset_size = 8;

void usage();
void error(const std::string& msg);

class parsing_error : public std::runtime_error {
public:
	parsing_error(const std::string& what_arg) : std::runtime_error {what_arg} {}
};

class assem_error : public std::runtime_error {
public:
	assem_error(const std::string& what_arg) : std::runtime_error {what_arg} {}
};

insmap_t insmap_gen(const std::string& conf_file);
oplist_t opvec_gen(std::ifstream& cfile, int numops);

std::string get_low_str(std::istream& infile);
std::string get_cfile_val(std::ifstream& cfile, const std::string& field_name);
void line_strip(std::string& line);
std::pair<tokvec_t, labelmap_t> tokenize_file(std::ifstream& infile, const insmap_t& insmap);
std::string ins2str(int pc, uint16_t iword);

uint16_t num_parse(const std::string& instr);

uint16_t reg_parse(const std::string& reg_name, std::unordered_map<std::string, std::string>& opfield_map, int pc);
uint16_t imm_parse(const std::string& imm_op, std::unordered_map<std::string, std::string>& opfield_map, int pc);
uint16_t label_parse(const std::string& label, std::unordered_map<std::string, std::string>& opfield_map, int pc);
uint16_t lit_parse(const std::string& op, std::unordered_map<std::string, std::string>& opfield_map, int pc);

bool reg_check(const std::string& op, std::unordered_map<std::string, std::string>& opmap);
bool imm_check(const std::string& op, std::unordered_map<std::string, std::string>& opmap);
bool label_check(const std::string& op, std::unordered_map<std::string, std::string>& opmap);
bool lit_check(const std::string& op, std::unordered_map<std::string, std::string>& opmap);

std::unordered_map<std::string, std::string> reg_opgen(std::ifstream& cfile);
std::unordered_map<std::string, std::string> imm_opgen(std::ifstream& cfile);
std::unordered_map<std::string, std::string> lit_opgen(std::ifstream& cfile);
std::unordered_map<std::string, std::string> no_opgen(std::ifstream& cfile);

extern std::unordered_map<std::string, int> label_map;
#endif
