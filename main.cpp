// Trabalho realizado pelos alunos da UFJF
// Ana Beatriz Kapps - 201835006
// Felipe Israel de Oliveira Vidal - 201835041

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <bitset>
#include <vector>
#include <string>
#include <stdlib.h>

using namespace std;

struct Control {
    int regDst;
    int branch;
    int memRead;
    int memtoReg;
    int aluOp0;
    int aluOp1;
    int memWrite;
    int aluSrc;
    int regWrite;
    int jump;
    int jumpReg;
};

int bancoRegistradores[32];
int qtdBancoRegistradores = 32;
int memoryData[64];
int qtdMemoryData = 64;
int instructionMemory[256];
int contInstructionMem = 0;
int PC;
Control control;

int zeroALU;

int opcode;
int aluControl;

int si;

unsigned executionType;
unsigned debugOption;

fstream textFile;

template<typename T>
std::string bstring(T n, int tam){
    std::string s;
    for(int m = tam;m--;){
        s.push_back('0'+((n >> m) & 1));
    }
    return s;
}

int data_memory(int address, int writeData) {
    if (control.memWrite == 1) {
        memoryData[address] = bancoRegistradores[writeData];
        return NULL;
    }
    return memoryData[address];
}

void printBancoRegistradores() {
    cout << "Banco de Registradores: ";
    for (int i = 0; i < qtdBancoRegistradores; i++) {
        cout << bancoRegistradores[i] << " ";
    }
    cout << endl;
}

void printDataMemory() {
    for (int i = 0; i < 32; i++) {
        cout << memoryData[i] << " ";
    }
    cout << endl << "-----------------------------" << endl;
}

void printInstructionMemory() {
    for (int i = 0; i < contInstructionMem; i++) {
        cout << instructionMemory[i] << " ";
    }
    cout << endl << "-----------------------------" << endl;
}

int alu(int readData1, int readData2) {
    if (control.aluOp0 == 0 && control.aluOp1 == 1) {
        // Instruções tipo-R
        // Devemos olhar para o funct
        int resultado = 0;
        switch (aluControl) {
            case 0:
                resultado = bancoRegistradores[readData2] << readData1;
                break;
            case 8:
                resultado = bancoRegistradores[readData1];
                break;
            case 24:
                resultado = bancoRegistradores[readData1] * bancoRegistradores[readData2];
                break;
            case 26:
                resultado = bancoRegistradores[readData1] / bancoRegistradores[readData2];
                break;
            case 32:
                resultado = bancoRegistradores[readData1] + bancoRegistradores[readData2];
                break;
            case 34:
                resultado = bancoRegistradores[readData1] - bancoRegistradores[readData2];
                break;
            case 36:
                resultado = bancoRegistradores[readData1] & bancoRegistradores[readData2];
                break;
            case 37:
                resultado = bancoRegistradores[readData1] | bancoRegistradores[readData2];
                break;
            case 42:
                resultado = bancoRegistradores[readData1] < bancoRegistradores[readData2] ? 1 : 0;
                break;
        }
        
        return resultado;
    } else if (control.aluOp0 == 0 && control.aluOp1 == 0) {
        // Instrução lw, sw ou addi
        
        int resultado = 0;
        switch (opcode) {
            case 8: { // addi
                resultado = bancoRegistradores[readData1] + readData2;
                break;
            }
            case 35: { // lw
                resultado = memoryData[readData1 + readData2];
                break;
            }
            case 43: { // sw
                resultado = readData1 + readData2;
                break;
            }
        }
        return resultado;
    } else if (control.aluOp0 == 1 && control.aluOp1 == 0) { // branch
        int result = bancoRegistradores[readData1] - bancoRegistradores[readData2];
        if (result == 0) {
            zeroALU = 1;
        } else {
            zeroALU = 0;
        }
        return result;
    }
    return 0;
}

string decToBinary(int n)
{
    // array to store binary number
    int binaryNum[32];
  
    // counter for binary array
    int i = 0;
    while (n > 0) {
  
        // storing remainder in binary array
        binaryNum[i] = n % 2;
        n = n / 2;
        i++;
    }
  
    // printing binary array in reverse order
    string bin;
    for (int j = i - 1; j >= 0; j--) {
        bin += to_string(binaryNum[j]);
    }
    return bin;
}

long binToDecimal(string binario, bool verificarSinal) {
    long result;
    if (verificarSinal) {
        int sign = binario[0] - '0';
        binario[0] = '0';
        result = stol(binario, 0, 2);
        if (sign == 1) {
            result = -result;
        }
    } else {
        result = stol(binario, 0, 2);
    }
    return result;
}

int extractBits(int decimalNumber, int inicio, int count) {
    string mask = "00000000000000000000000000000000";
    for (int i = inicio; i < inicio + count; i++) {
        mask[i] = '1';
    }

    long decimalMask = binToDecimal(mask, false);
    uint32_t resultado = decimalNumber & decimalMask;

    int shamt = abs(32 - (inicio + count));
    int resultadoFinal = resultado >> shamt;
    return resultadoFinal;
}

void registers(int readRegister1, int readRegister2, int writeRegister, int* writeData) {
    
    int resultadoALU;
    if (control.aluSrc == 0) {
        resultadoALU = alu(readRegister1, readRegister2);
    } else {
        resultadoALU = alu(readRegister1, si);
    }

    if (control.jumpReg == 1) {
        PC = resultadoALU;
        return;
    } else {
        long memtoRegDec = binToDecimal(to_string(control.memtoReg), false);
        if (memtoRegDec == 2) {
            resultadoALU = PC + 1;
        }

        if (writeData != NULL) {
            *writeData = resultadoALU;
        }
            
        if (control.regWrite == 1) {
            bancoRegistradores[writeRegister] = *writeData;
        }

        data_memory(resultadoALU, writeRegister);
    }
    
}

int signExtended(int immediate) {

    string result = bstring<size_t>(immediate, 32);

    if (immediate < 0) {
        result[0] = '1';
    }

//    cout << "Sinal Extendido: " << result << endl;

    return immediate;
}

void instruction_memory(int read_address) {
    int instruction = instructionMemory[read_address];
    
    opcode = extractBits(instruction, 0, 6);
    
    if (opcode == 0) {
        // Função do tipo-R
        
        control.jump = 0;
        control.regDst = 0;
        control.branch = 0;
        control.memRead = 0;
        control.memtoReg = 0;
        control.aluOp0 = 0;
        control.aluOp1 = 1;
        control.memWrite = 0;
        control.aluSrc = 0;
        control.regWrite = 1;
        control.regDst = 1;
        
        int funct = extractBits(instruction, 26, 6);
        aluControl = funct;

        if (aluControl == 8) {
            control.jumpReg = 1;
        } else {
            control.jumpReg = 0;
        }
        
    } else {
        // Função do tipo-I ou tipo-J
        
        if (opcode == 2) { // j
            
            control.jump = 1;
            control.branch = 1;
            control.jumpReg = 0;
            
            int address = extractBits(instruction, 6, 26);
            
            address = address << 2;
            
            string binAddress = bstring<size_t>(address, 28);
            
            string binPC = bstring<size_t>(PC, 32);
            
            binAddress.insert(0, binPC.substr(0, 4));
            
            if (control.jump == 1) {
                int finalAddress = (int)binToDecimal(binAddress, true);
                PC = finalAddress / 4;
                
                return;
            }
            
        } else if (opcode == 3) { // jal
            
            control.jump = 1;
            control.regDst = 0;
            control.branch = 1;
            control.memRead = 0;
            control.memtoReg = 10;
            control.aluOp0 = 0;
            control.aluOp1 = 0;
            control.memWrite = 0;
            control.aluSrc = 0;
            control.regWrite = 1;
            control.regDst = 10;
            control.jumpReg = 0;
            
        } else {
            
            int immediate = extractBits(instruction, 16, 16);
            
            switch (opcode) {
                case 4: case 5: {
                    // branch
                    
                    control.regDst = 0;
                    control.branch = 1;
                    control.memRead = 0;
                    control.memtoReg = 0;
                    control.aluOp0 = 1;
                    control.aluOp1 = 0;
                    control.memWrite = 0;
                    control.aluSrc = 0;
                    control.regWrite = 0;
                    control.regDst = 0;
                    control.jumpReg = 0;
                    
                    break;
                }
                case 8:
                    // addi
                    
                    control.branch = 0;
                    control.memRead = 0;
                    control.memtoReg = 0;
                    control.aluOp0 = 0;
                    control.aluOp1 = 0;
                    control.memWrite = 0;
                    control.aluSrc = 1;
                    control.regWrite = 1;
                    control.regDst = 0;
                    control.jumpReg = 0;
                    
                    si = signExtended(immediate);
                    
                    break;
                case 35: {
                    // lw
                    
                    control.regDst = 0;
                    control.branch = 0;
                    control.memRead = 1;
                    control.memtoReg = 1;
                    control.aluOp0 = 0;
                    control.aluOp1 = 0;
                    control.memWrite = 0;
                    control.aluSrc = 1;
                    control.regWrite = 1;
                    control.regDst = 0;
                    control.jumpReg = 0;

                    si = signExtended(immediate);
                    
                    break;
                }
                case 43: {
                    // sw
                    
                    control.regDst = 0;
                    control.branch = 0;
                    control.memRead = 0;
                    control.memtoReg = 0;
                    control.aluOp0 = 0;
                    control.aluOp1 = 0;
                    control.memWrite = 1;
                    control.aluSrc = 1;
                    control.regWrite = 0;
                    control.regDst = 0;
                    control.jumpReg = 0;
                    
                    si = signExtended(immediate);
                    
                    break;
                }
            }
            
        }
    }
    
    int writeRegister;
    
    // Mulitplexador
    long regDstDec = binToDecimal(to_string(control.regDst), false);
    switch (regDstDec) {
        case 0:
            writeRegister = extractBits(instruction, 11, 5); // rt
            break;
        case 1:
            writeRegister = extractBits(instruction, 16, 5); // rd
            break;
        case 2:
            writeRegister = 31;
            break;
        default:
            break;
    }
    
    int rs = extractBits(instruction, 6, 5);
    int rt = extractBits(instruction, 11, 5);
    
    int writeData;
    
    if (opcode == 0 && aluControl == 0) {
        int shamt = extractBits(instruction, 21, 5);
        registers(shamt, rt, writeRegister, &writeData);
    } else {
        registers(rs, rt, writeRegister, &writeData);
        
        if (opcode == 4 || opcode == 5) {
            int immediate = extractBits(instruction, 16, 16);
            si = signExtended(immediate);
        } else if (opcode == 3) {
            int address = extractBits(instruction, 6, 26);
            
            address = address << 2;
            
            string binAddress = bstring<size_t>(address, 28);
            
            string binPC = bstring<size_t>(PC, 32);
            
            binAddress.insert(0, binPC.substr(0, 4));
            
            // si = signExtended(address);
            
            if (control.jump == 1) {
                int finalAddress = (int)binToDecimal(binAddress, true);
                PC = finalAddress / 4;
                cout << "Próx PC = " << finalAddress << endl;

                return;
            }
        }
    }
}

void printResults() {
    // Banco registradores
    cout << "Banco de Registradores: ";
    for (int i = 0; i < qtdBancoRegistradores; i++) {
        cout << bancoRegistradores[i] << " ";
    }
    cout << endl;

    // memória de dados
    cout << "Memória de Dados: ";
    for (int i = 0; i < qtdMemoryData; i++) {
        if (memoryData[i] == INT_MAX) {
            cout << "NAN" << " ";
        } else {
            cout << memoryData[i] << " ";
        }
    }
    cout << endl;

    cout << endl << "Sinais de Controle: " << endl;
    cout << "- regDst: " << control.regDst << endl;
    cout << "- branch: " << control.branch << endl;
    cout << "- memRead: " << control.memRead << endl;
    cout << "- memtoReg: " << control.memtoReg << endl;
    cout << "- aluOp0: " << control.aluOp0 << endl;
    cout << "- aluOp1: " << control.aluOp1 << endl;
    cout << "- memWrite: " << control.memWrite << endl;
    cout << "- aluSrc: " << control.aluSrc << endl;
    cout << "- regWrite: " << control.regWrite << endl;
    cout << "- jump: " << control.jump << endl;
    cout << "- jumpReg: " << control.jumpReg << endl;

    cout << endl;
}

void writeResults() {
    // Banco registradores
    textFile << "Banco de Registradores: ";
    for (int i = 0; i < qtdBancoRegistradores; i++) {
        textFile << bancoRegistradores[i] << " ";
    }
    textFile << endl;

    // memória de dados
    textFile << "Memória de Dados: ";
    for (int i = 0; i < qtdMemoryData; i++) {
        if (memoryData[i] == INT_MAX) {
            textFile << "NAN" << " ";
        } else {
            textFile << memoryData[i] << " ";
        }
    }
    textFile << endl;

    // Sinais de controle
    textFile << endl << "Sinais de Controle: " << endl;
    textFile << "- regDst: " << control.regDst << endl;
    textFile << "- branch: " << control.branch << endl;
    textFile << "- memRead: " << control.memRead << endl;
    textFile << "- memtoReg: " << control.memtoReg << endl;
    textFile << "- aluOp0: " << control.aluOp0 << endl;
    textFile << "- aluOp1: " << control.aluOp1 << endl;
    textFile << "- memWrite: " << control.memWrite << endl;
    textFile << "- aluSrc: " << control.aluSrc << endl;
    textFile << "- regWrite: " << control.regWrite << endl;
    textFile << "- jump: " << control.jump << endl;
    textFile << "- jumpReg: " << control.jumpReg << endl;

    textFile << endl;
}

void printFinalResult() {
    cout << endl << "========== Resultado Final: ==========" << endl;

    cout << "Banco de Registradores: ";
    for (int i = 0; i < qtdBancoRegistradores; i++) {
        cout << bancoRegistradores[i] << " ";
    }
    cout << endl;

    // memória de dados
    cout << "Memória de Dados: ";
    for (int i = 0; i < qtdMemoryData; i++) {
        if (memoryData[i] == INT_MAX) {
            cout << "NAN" << " ";
        } else {
            cout << memoryData[i] << " ";
        }
    }
    cout << endl;

    // Sinais de controle
    cout << endl << "Sinais de Controle: " << endl;
    cout << "- regDst: " << control.regDst << endl;
    cout << "- branch: " << control.branch << endl;
    cout << "- memRead: " << control.memRead << endl;
    cout << "- memtoReg: " << control.memtoReg << endl;
    cout << "- aluOp0: " << control.aluOp0 << endl;
    cout << "- aluOp1: " << control.aluOp1 << endl;
    cout << "- memWrite: " << control.memWrite << endl;
    cout << "- aluSrc: " << control.aluSrc << endl;
    cout << "- regWrite: " << control.regWrite << endl;
    cout << "- jump: " << control.jump << endl;
    cout << "- jumpReg: " << control.jumpReg << endl;

    cout << endl;
}

void writeFinalResult() {
    textFile << endl << "========== Resultado Final: ==========" << endl;

    textFile << "Banco de Registradores: ";
    for (int i = 0; i < qtdBancoRegistradores; i++) {
        textFile << bancoRegistradores[i] << " ";
    }
    textFile << endl;

    // memória de dados
    textFile << "Memória de Dados: ";
    for (int i = 0; i < qtdMemoryData; i++) {
        if (memoryData[i] == INT_MAX) {
            textFile << "NAN" << " ";
        } else {
            textFile << memoryData[i] << " ";
        }
    }
    textFile << endl;

    // Sinais de controle
    textFile << endl << "Sinais de Controle: " << endl;
    textFile << "- regDst: " << control.regDst << endl;
    textFile << "- branch: " << control.branch << endl;
    textFile << "- memRead: " << control.memRead << endl;
    textFile << "- memtoReg: " << control.memtoReg << endl;
    textFile << "- aluOp0: " << control.aluOp0 << endl;
    textFile << "- aluOp1: " << control.aluOp1 << endl;
    textFile << "- memWrite: " << control.memWrite << endl;
    textFile << "- aluSrc: " << control.aluSrc << endl;
    textFile << "- regWrite: " << control.regWrite << endl;
    textFile << "- jump: " << control.jump << endl;
    textFile << "- jumpReg: " << control.jumpReg << endl;

    textFile << endl;
}

void executeInstruction() {
    instruction_memory(PC);
            
    if (executionType == 0) {
        textFile << "---------------------------------------" << endl;
        textFile << "PC: " << PC << endl;

        cout << "---------------------------------------" << endl;
        cout << "PC: " << PC << endl;
        
        writeResults();
        printResults();
    }
    
    if (control.branch == 0 && control.jumpReg == 0) {
        PC++;
    } else {
        int resultado_si = si << 2;
        resultado_si /= 4;
        
        if (opcode == 4) {
            int signControl = control.branch & zeroALU;
            if (signControl == 1) {
                PC += resultado_si;
            } else {
                PC++;
            }
        } else if (opcode == 5) {
            int signControl = zeroALU & control.branch;
            if (signControl == 1) {
                PC++;
            } else {
                PC += resultado_si;
            }
        }
    }
}

void startExecution() {

    textFile.open("relatorio.txt", ios::out);
    
    cout << "============ TIPO DE EXECUÇÃO ============" << endl;
    cout << "0. Passo a passo" << endl;
    cout << "1. Direta" << endl;

    cout << "Opção: ";
    cin >> executionType;
    
    while (executionType != 0 && executionType != 1) {
        cout << "Opção Inválida." << endl;
        cout << "Opção: ";
        cin >> executionType;
    }

    if (executionType == 0) {
        cout << "=========== ATIVAR MODO DEBUG? ===========" << endl;
        cout << "0. Não" << endl;
        cout << "1. Sim" << endl;

        cout << "Opção: ";
        cin >> debugOption;

        while (debugOption != 0 && debugOption != 1) {
            cout << "Opção Inválida." << endl;
            cout << "Opção: ";
            cin >> debugOption;
        }
    }

    if (executionType == 0) {
        cout << "========== EXECUÇÃO PASSO A PASSO ==========" << endl;
    } else {
        cout << "========== EXECUÇÃO DIRETA ==========" << endl;
    }
    
    PC = 0;

    // Iniciar banco de registradores
    for (int i = 0; i < qtdBancoRegistradores; i++) {
        bancoRegistradores[i] = 0;
    }
    
    // Iniciar memória de dados
    for (int i = 0; i < qtdMemoryData; i++) {
        memoryData[i] = INT_MAX;
    }
    
    control.regDst = 0;
    control.branch = 0;
    control.memRead = 0;
    control.memtoReg = 0;
    control.aluOp0 = 0;
    control.aluOp1 = 0;
    control.memWrite = 0;
    control.aluSrc = 0;
    control.regWrite = 0;
    control.jump = 0;
    control.jumpReg = 0;
    
    while (PC < contInstructionMem) {
        // cout << "PC = " << PC << endl;

        if (debugOption == 0) {
            executeInstruction();
        } else {
            // Perguntar
            unsigned debugProx;
            cout << "Realizar próxima instrução (PC = " << PC << ")? (0 - Não / 1 - Sim) ";
            cin >> debugProx;

            while (debugProx == 0) {
                cout << "Realizar próxima instrução (PC = " << PC << ")? (0 - Não / 1 - Sim) ";
                cin >> debugProx;
            }
            
            executeInstruction();
        }
    }

    // Escrever no arquivo e imprimir o resultado final
    writeFinalResult();
    printFinalResult();

    // Stop execution
    textFile.close();
}

void readFile(string filePath) {
    ifstream file;
    file.open(filePath);

    if (file.is_open()) {
        while (!file.eof()) {
            if (contInstructionMem < 256) {
                string str; // número binário
                getline(file, str);

                // Converter as string binária para decimal e armazenar na memória de dados
                uint32_t number = stol(str, 0, 2); // número decimal
                instructionMemory[contInstructionMem] = number;
                contInstructionMem++;
            } else {
                cout << "Memória Cheia!" << endl;
                exit(0);
            }
        }
        
        startExecution();
    }
    
    file.close();
}

void readInput() {
    
    cout << "========== DIGITE AS INSTRUÇÕES ==========" << endl;
    cout << "Manual:" << endl;
    cout << "(-) Apagar instrução anterior" << endl;
    cout << "(0) Finalizar e Iniciar Execução" << endl;
    cout << "------------------------------------------" << endl;
    
    string line;
    while (line != "0") {
        cin >> line;
        if (line != "0") {
            if (line == "-") {
                instructionMemory[contInstructionMem - 1] = 0;
                contInstructionMem--;
            } else {
                uint32_t number = stol(line, 0, 2); // número decimal
                instructionMemory[contInstructionMem] = number;
                contInstructionMem++;
            }
        }
    }
    
    printInstructionMemory();
    
    startExecution();
}

int main() {

    unsigned option;

    cout << "================== MIPS ==================" << endl;
    cout << "=========== CAMINHO DO ARQUIVO ===========" << endl;
    cout << "Como você deseja iniciar a execução?" << endl;
    cout << "0. Carregar arquivo" << endl;
    cout << "1. Digitar instruções" << endl;
    
    cout << "Opção: ";
    cin >> option;
    
    while (option != 0 && option != 1) {
        cout << "Opção Inválida." << endl;
        cout << "Opção: ";
        cin >> option;
    }

    switch (option) {
    case 0: {
        string filePath;
        cout << "=========== CAMINHO DO ARQUIVO ===========" << endl;
        cout << "Caminho: ";
        cin >> filePath;
        
        readFile(filePath);
        
        break;
    }
    case 1: {
        readInput();
        break;
    }
    default:
        break;
    }

    return 0;
}
