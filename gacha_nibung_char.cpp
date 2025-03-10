#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <ctime>
#include <limits>
#include <iomanip>
#include <map>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

// Struktur untuk menyimpan hasil pull
struct GachaResult {
    std::string item;
    std::string rarity;
    bool isPity;
    int pullNumber;
};

// Struktur untuk karakter
struct Character {
    std::string name;
    std::string rarity; // SSR, SR, R, Common
    double rate;
    std::string title;
    std::string element; // Elemen karakter (baru)
};

// Enumerasi untuk warna konsol (Windows)
enum ConsoleColor {
    BLACK = 0,
    BLUE = 1,
    GREEN = 2,
    CYAN = 3,
    RED = 4,
    MAGENTA = 5,
    YELLOW = 6,
    WHITE = 7,
    BRIGHT = 8
};

// Fungsi untuk mengatur warna konsol (Windows)
void setConsoleColor(ConsoleColor textColor, ConsoleColor bgColor = BLACK) {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, (bgColor << 4) | textColor);
#else
    // Untuk sistem UNIX/Linux, gunakan ANSI escape codes
    switch(textColor) {
        case BLACK: std::cout << "\033[30m"; break;
        case RED: std::cout << "\033[31m"; break;
        case GREEN: std::cout << "\033[32m"; break;
        case YELLOW: std::cout << "\033[33m"; break;
        case BLUE: std::cout << "\033[34m"; break;
        case MAGENTA: std::cout << "\033[35m"; break;
        case CYAN: std::cout << "\033[36m"; break;
        case WHITE: std::cout << "\033[37m"; break;
        default: std::cout << "\033[0m"; break;
    }
#endif
}

// Fungsi untuk mereset warna konsol
void resetConsoleColor() {
#ifdef _WIN32
    setConsoleColor(WHITE, BLACK);
#else
    std::cout << "\033[0m";
#endif
}

class GachaSystem {
private:
    std::vector<Character> characters;
    int hardPity;           // Garansi SSR (biasanya 100)
    int softPityStart;      // Kapan soft pity mulai (biasanya 75)
    double softPityBoost;   // Faktor peningkatan rate
    int pullCount;
    int selectedCharPity;   // Indeks karakter yang akan didapat saat pity
    std::vector<GachaResult> history;
    std::mt19937 rng;
    std::uniform_real_distribution<double> dist;
    std::map<std::string, double> rarityRates; // Rate untuk setiap rarity
    
    // Cache untuk total rate setiap rarity
    std::map<std::string, double> totalRarityRates;

public:
    // Konstruktor
    GachaSystem() : 
        hardPity(90),       // Garansi pada pull ke-90
        softPityStart(75),  // Soft pity mulai pada pull ke-75
        softPityBoost(5.0), // 5x boost saat soft pity
        pullCount(0),
        selectedCharPity(0) {
        
        // Inisialisasi generator angka random
        std::random_device rd;
        rng = std::mt19937(rd());
        dist = std::uniform_real_distribution<double>(0.0, 1.0);
        
        // Set rate default untuk setiap rarity
        rarityRates["SSR"] = 0.01;  // 1%
        rarityRates["SR"] = 0.05;   // 5%
        rarityRates["R"] = 0.15;    // 15%
        rarityRates["Common"] = 0.79; // 79%
        
        // Inisialisasi total rate
        recalculateTotalRates();
    }
    
    // Recalculate total rates for each rarity
    void recalculateTotalRates() {
        totalRarityRates.clear();
        
        for (const auto& character : characters) {
            totalRarityRates[character.rarity] += character.rate;
        }
    }
    
    // Menambahkan karakter baru
    void addCharacter(const std::string& name, const std::string& rarity, double rate, 
                    const std::string& title = "", const std::string& element = "") {
        Character character;
        character.name = name;
        character.rarity = rarity;
        character.rate = rate;
        character.title = title;
        character.element = element;
        
        characters.push_back(character);
        
        // Update total rates
        recalculateTotalRates();
    }
    
    // Mengatur parameter pity
    void setPitySettings(int hardPityValue, int softPityValue, double softPityBoostValue) {
        if (hardPityValue > 0 && softPityValue > 0 && softPityValue < hardPityValue && softPityBoostValue > 1.0) {
            hardPity = hardPityValue;
            softPityStart = softPityValue;
            softPityBoost = softPityBoostValue;
        }
    }
    
    // Set karakter pity
    void setSelectedCharPity(int index) {
        if (index >= 0 && index < static_cast<int>(characters.size())) {
            selectedCharPity = index;
        }
    }
    
    // Set karakter pity berdasarkan nama
    bool setSelectedCharPityByName(const std::string& name) {
        for (size_t i = 0; i < characters.size(); i++) {
            if (characters[i].name == name && characters[i].rarity == "SSR") {
                selectedCharPity = i;
                return true;
            }
        }
        return false;
    }

    // Melakukan satu kali pull
    GachaResult pull() {
        pullCount++;
        GachaResult result;
        
        // Periksa apakah ini adalah hard pity
        if (pullCount >= hardPity) {
            result.item = characters[selectedCharPity].name;
            result.rarity = "SSR";
            result.isPity = true;
            result.pullNumber = pullCount;
            pullCount = 0; // Reset counter setelah mendapat garansi
        } else {
            // Cek jika dalam kondisi soft pity
            bool inSoftPity = (pullCount >= softPityStart);
            double ssrRateMultiplier = inSoftPity ? softPityBoost : 1.0;
            
            // Perhitungan normal dengan rate untuk masing-masing karakter
            double randomValue = dist(rng);
            double cumulativeRate = 0.0;
            
            // Tentukan rarity terlebih dahulu
            std::string selectedRarity = "Common";
            double rarityRand = dist(rng);
            double raritySum = 0.0;
            
            // Menentukan rarity terlebih dahulu (SSR rate dipengaruhi soft pity)
            double adjustedSSRRate = rarityRates["SSR"] * ssrRateMultiplier;
            double totalRate = adjustedSSRRate + rarityRates["SR"] + rarityRates["R"] + rarityRates["Common"];
            
            // Normalisasi rate untuk total 1.0
            double normalizedSSRRate = adjustedSSRRate / totalRate;
            double normalizedSRRate = rarityRates["SR"] / totalRate;
            double normalizedRRate = rarityRates["R"] / totalRate;
            
            if (rarityRand < normalizedSSRRate) {
                selectedRarity = "SSR";
            } else if (rarityRand < normalizedSSRRate + normalizedSRRate) {
                selectedRarity = "SR";
            } else if (rarityRand < normalizedSSRRate + normalizedSRRate + normalizedRRate) {
                selectedRarity = "R";
            } else {
                selectedRarity = "Common";
            }
            
            // Jika mendapat SSR, reset pity counter
            bool resetCounter = (selectedRarity == "SSR");
            
            // Pilih karakter spesifik dari rarity terpilih
            std::vector<Character> rarityChars;
            double totalRarityRate = 0.0;
            
            for (const auto& character : characters) {
                if (character.rarity == selectedRarity) {
                    rarityChars.push_back(character);
                    totalRarityRate += character.rate;
                }
            }
            
            // Default jika tidak ada karakter untuk rarity tersebut
            if (rarityChars.empty()) {
                result.item = selectedRarity + " Item";
                result.rarity = selectedRarity;
                result.isPity = false;
                result.pullNumber = pullCount;
            } else {
                // Pilih karakter dari rarity terpilih
                double charRand = dist(rng) * totalRarityRate;
                double charCumRate = 0.0;
                
                for (const auto& character : rarityChars) {
                    charCumRate += character.rate;
                    if (charRand <= charCumRate) {
                        result.item = character.name;
                        result.rarity = character.rarity;
                        result.isPity = false;
                        result.pullNumber = pullCount;
                        break;
                    }
                }
            }
            
            // Reset counter jika mendapat SSR
            if (resetCounter) {
                pullCount = 0;
            }
        }
        
        history.push_back(result);
        return result;
    }
    
    // Melakukan multiple pull
    std::vector<GachaResult> multiPull(int count) {
        std::vector<GachaResult> results;
        for (int i = 0; i < count; i++) {
            results.push_back(pull());
        }
        return results;
    }
    
    // Mendapatkan history pull
    const std::vector<GachaResult>& getHistory() const {
        return history;
    }
    
    // Mendapatkan berapa pull lagi sampai garansi
    int getPityCounter() const {
        return hardPity - pullCount;
    }
    
    // Mendapatkan informasi soft pity
    int getSoftPityCounter() const {
        return softPityStart - pullCount;
    }
    
    // Cek apakah dalam kondisi soft pity
    bool isInSoftPity() const {
        return pullCount >= softPityStart && pullCount < hardPity;
    }
    
    // Mendapatkan karakter yang akan didapat saat pity
    std::string getSelectedPityCharName() const {
        if (selectedCharPity >= 0 && selectedCharPity < static_cast<int>(characters.size())) {
            return characters[selectedCharPity].name;
        }
        return "Unknown";
    }
    
    // Mendapatkan karakter berdasarkan rarity
    std::vector<Character> getCharactersByRarity(const std::string& rarity) const {
        std::vector<Character> filteredChars;
        for (const auto& character : characters) {
            if (character.rarity == rarity) {
                filteredChars.push_back(character);
            }
        }
        return filteredChars;
    }
    
    // Mendapatkan daftar karakter SSR
    std::vector<Character> getSSRCharacters() const {
        return getCharactersByRarity("SSR");
    }
    
    // Mendapatkan daftar semua karakter
    const std::vector<Character>& getAllCharacters() const {
        return characters;
    }
    
    // Mendapatkan rate berdasarkan rarity
    double getRarityRate(const std::string& rarity) const {
        auto it = rarityRates.find(rarity);
        if (it != rarityRates.end()) {
            return it->second;
        }
        return 0.0;
    }
    
    // Mendapatkan informasi rate saat ini (dengan soft pity)
    double getCurrentSSRRate() const {
        if (isInSoftPity()) {
            return rarityRates.at("SSR") * softPityBoost;
        }
        return rarityRates.at("SSR");
    }
    
    // Menampilkan hasil pull dengan warna
    static void printResult(const GachaResult& result, const std::vector<Character>& allCharacters) {
        // Set warna berdasarkan rarity
        if (result.rarity == "SSR") {
            setConsoleColor(YELLOW, BLACK);
        } else if (result.rarity == "SR") {
            setConsoleColor(MAGENTA, BLACK);
        } else if (result.rarity == "R") {
            setConsoleColor(CYAN, BLACK);
        } else {
            setConsoleColor(WHITE, BLACK);
        }
        
        std::cout << "Item: " << result.item;
        
        // Tambahkan title dan elemen jika ada
        for (const auto& character : allCharacters) {
            if (character.name == result.item) {
                if (!character.title.empty()) {
                    std::cout << " - " << character.title;
                }
                if (!character.element.empty()) {
                    std::cout << " (" << character.element << ")";
                }
                break;
            }
        }
        
        std::cout << ", Rarity: " << result.rarity;
        
        // Tampilkan bintang berdasarkan rarity
        if (result.rarity == "SSR") {
            std::cout << " ★★★★★";
        } else if (result.rarity == "SR") {
            std::cout << " ★★★★";
        } else if (result.rarity == "R") {
            std::cout << " ★★★";
        }
        
        if (result.isPity) {
            std::cout << " (GUARANTEED PITY!)";
        }
        
        std::cout << ", Pull #: " << result.pullNumber;
        
        resetConsoleColor();
        std::cout << std::endl;
    }
    
    // Menghitung jumlah karakter berdasarkan rarity yang didapat
    std::map<std::string, std::map<std::string, int>> countCharactersByRarity() const {
        std::map<std::string, std::map<std::string, int>> counts;
        for (const auto& result : history) {
            counts[result.rarity][result.item]++;
        }
        return counts;
    }
};

// Fungsi untuk membersihkan layar
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// Fungsi untuk menampilkan menu
void displayMenu() {
    clearScreen();
    std::cout << "===================================\n";
    std::cout << "          GACHA SYSTEM            \n";
    std::cout << "===================================\n";
    std::cout << "1. Gacha 1x\n";
    std::cout << "2. Gacha 10x\n";
    std::cout << "3. Cek Pity Counter\n";
    std::cout << "4. Lihat Riwayat Gacha\n";
    std::cout << "5. Ganti Karakter Pity\n";
    std::cout << "6. Lihat Daftar Karakter\n";
    std::cout << "7. Lihat Info Rate\n";
    std::cout << "8. Simulasi Gacha (Sampai dapat SSR)\n";
    std::cout << "0. Keluar\n";
    std::cout << "===================================\n";
    std::cout << "Pilihan Anda: ";
}

// Fungsi untuk mendapatkan input yang valid
int getValidInput(int min, int max) {
    int choice;
    std::cin >> choice;
    
    while (std::cin.fail() || choice < min || choice > max) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Input tidak valid. Silakan masukkan angka " << min << "-" << max << ": ";
        std::cin >> choice;
    }
    
    return choice;
}

int main() {
    // Inisialisasi sistem gacha
    GachaSystem gachaSystem;
    
    // Tambahkan karakter
    // SSR Characters
    gachaSystem.addCharacter("razib", "SSR", 0.004, "The great dancer", "water");
    gachaSystem.addCharacter("Dappupu", "SSR", 0.004, "Lord of Nibung", "Earth");
    gachaSystem.addCharacter("aulia", "SSR", 0.002, "the dark ciken wing", "Dark");
    gachaSystem.addCharacter("oby", "SSR", 0.004, "the killer coboy", "steal" );
    gachaSystem.addCharacter("ippanIcikiwir", "SSR", 0.004, "the Great Hook rider", "Flame" );
    gachaSystem.addCharacter("Yahahawahyu", "SSR", 0.004, "the laughty disaster", "aki" );
    
    // SR Characters
    gachaSystem.addCharacter("Axel", "SR", 0.015, "Pyro Knight", "Fire");
    gachaSystem.addCharacter("Luna", "SR", 0.015, "Moonlight Archer", "Light");
    gachaSystem.addCharacter("Kai", "SR", 0.01, "Ocean Guardian", "Water");
    gachaSystem.addCharacter("Riona", "SR", 0.01, "Nature's Embrace", "Earth");
    
    // R Characters
    gachaSystem.addCharacter("Thorne", "R", 0.03, "Shadow Blade", "Dark");
    gachaSystem.addCharacter("Lilith", "R", 0.03, "Flame Dancer", "Fire");
    gachaSystem.addCharacter("Gale", "R", 0.03, "Swift Scout", "Wind");
    gachaSystem.addCharacter("Nami", "R", 0.03, "Tide Caller", "Water");
    gachaSystem.addCharacter("Spark", "R", 0.03, "Lightning Rod", "Thunder");
    
    int choice;
    
    do {
        displayMenu();
        choice = getValidInput(0, 8);
        
        clearScreen();
        
        switch (choice) {
            case 1: {
                // Gacha 1x
                std::cout << "Melakukan Gacha 1x...\n\n";
                GachaResult result = gachaSystem.pull();
                GachaSystem::printResult(result, gachaSystem.getAllCharacters());
                
                if (result.rarity == "SSR") {
                    std::cout << "\n*** SELAMAT! Anda mendapatkan karakter SSR! ***\n";
                } else if (result.rarity == "SR") {
                    std::cout << "\n** Bagus! Anda mendapatkan karakter SR! **\n";
                }
                break;
            }
            case 2: {
                // Gacha 10x
                std::cout << "Melakukan Gacha 10x...\n\n";
                std::vector<GachaResult> results = gachaSystem.multiPull(10);
                
                bool gotSSR = false;
                bool gotSR = false;
                
                for (size_t i = 0; i < results.size(); i++) {
                    std::cout << "Pull " << (i + 1) << ": ";
                    GachaSystem::printResult(results[i], gachaSystem.getAllCharacters());
                    
                    if (results[i].rarity == "SSR") {
                        gotSSR = true;
                    } else if (results[i].rarity == "SR") {
                        gotSR = true;
                    }
                }
                
                if (gotSSR) {
                    std::cout << "\n*** SELAMAT! Anda mendapatkan karakter SSR! ***\n";
                } else if (gotSR) {
                    std::cout << "\n** Bagus! Anda mendapatkan karakter SR! **\n";
                }
                break;
            }
            case 3: {
                // Cek Pity Counter
                std::cout << "Status Pity Counter:\n";
                std::cout << "Pull tersisa sampai hard pity: " << gachaSystem.getPityCounter() << std::endl;
                
                int softPityCounter = gachaSystem.getSoftPityCounter();
                if (softPityCounter <= 0) {
                    std::cout << "Anda dalam kondisi soft pity! Rate SSR telah meningkat!" << std::endl;
                } else {
                    std::cout << "Pull tersisa sampai soft pity: " << softPityCounter << std::endl;
                }
                
                std::cout << "Rate SSR saat ini: " << (gachaSystem.getCurrentSSRRate() * 100) << "%" << std::endl;
                std::cout << "Karakter pity saat ini: " << gachaSystem.getSelectedPityCharName() << std::endl;
                break;
            }
            case 4: {
                // Lihat Riwayat Gacha
                const std::vector<GachaResult>& history = gachaSystem.getHistory();
                auto charCounts = gachaSystem.countCharactersByRarity();
                
                std::cout << "Riwayat Gacha:\n";
                std::cout << "Total pull dilakukan: " << history.size() << std::endl;
                
                std::cout << "\nKarakter yang didapatkan:\n";
                
                // Tampilkan SSR
                if (!charCounts["SSR"].empty()) {
                    setConsoleColor(YELLOW);
                    std::cout << "\nSSR Characters:\n";
                    resetConsoleColor();
                    
                    int totalSSR = 0;
                    for (const auto& pair : charCounts["SSR"]) {
                        std::cout << "- " << pair.first << ": " << pair.second << std::endl;
                        totalSSR += pair.second;
                    }
                    std::cout << "Total SSR: " << totalSSR << std::endl;
                }
                
                // Tampilkan SR
                if (!charCounts["SR"].empty()) {
                    setConsoleColor(MAGENTA);
                    std::cout << "\nSR Characters:\n";
                    resetConsoleColor();
                    
                    int totalSR = 0;
                    for (const auto& pair : charCounts["SR"]) {
                        std::cout << "- " << pair.first << ": " << pair.second << std::endl;
                        totalSR += pair.second;
                    }
                    std::cout << "Total SR: " << totalSR << std::endl;
                }
                
                // Tampilkan R
                if (!charCounts["R"].empty()) {
                    setConsoleColor(CYAN);
                    std::cout << "\nR Characters:\n";
                    resetConsoleColor();
                    
                    int totalR = 0;
                    for (const auto& pair : charCounts["R"]) {
                        std::cout << "- " << pair.first << ": " << pair.second << std::endl;
                        totalR += pair.second;
                    }
                    std::cout << "Total R: " << totalR << std::endl;
                }
                
                if (history.empty()) {
                    std::cout << "\nBelum ada riwayat gacha.\n";
                } else {
                    // Hanya tampilkan 20 riwayat terakhir untuk menghemat ruang
                    size_t startIdx = (history.size() > 20) ? history.size() - 20 : 0;
                    
                    std::cout << "\n20 Riwayat Gacha Terakhir:\n";
                    for (size_t i = startIdx; i < history.size(); i++) {
                        std::cout << "Pull #" << (i + 1) << ": ";
                        GachaSystem::printResult(history[i], gachaSystem.getAllCharacters());
                    }
                }
                break;
            }
            case 5: {
                // Ganti Karakter Pity
                auto ssrChars = gachaSystem.getSSRCharacters();
                
                std::cout << "Ganti Karakter Pity:\n\n";
                std::cout << "Pilih karakter untuk pity:\n";
                
                for (size_t i = 0; i < ssrChars.size(); i++) {
                    std::cout << (i + 1) << ". " << ssrChars[i].name;
                    if (!ssrChars[i].title.empty()) {
                        std::cout << " - " << ssrChars[i].title;
                    }
                    if (!ssrChars[i].element.empty()) {
                        std::cout << " (" << ssrChars[i].element << ")";
                    }
                    std::cout << " (Rate: " << (ssrChars[i].rate * 100) << "%)\n";
                }
                
                std::cout << "\nPilihan Anda: ";
                int charChoice = getValidInput(1, ssrChars.size());
                
                std::string selectedChar = ssrChars[charChoice - 1].name;
                gachaSystem.setSelectedCharPityByName(selectedChar);
                
                std::cout << "\nKarakter pity diubah menjadi: " << selectedChar << std::endl;
                break;
            }
            case 6: {
                // Lihat Daftar Karakter
                std::cout << "Daftar Karakter:\n\n";
                
                // Tampilkan SSR
                auto ssrChars = gachaSystem.getSSRCharacters();
                setConsoleColor(YELLOW);
                std::cout << "[ SSR Characters (Rate: " << (gachaSystem.getRarityRate("SSR") * 100) << "%) ]\n";
                resetConsoleColor();
                
                std::cout << std::left << std::setw(15) << "Nama" << std::setw(20) << "Title" 
                        << std::setw(10) << "Element" << "Rate" << std::endl;
                std::cout << std::string(60, '-') << std::endl;
                
                for (const auto& character : ssrChars) {
                    std::cout << std::left << std::setw(15) << character.name 
                            << std::setw(20) << character.title 
                            << std::setw(10) << character.element
                            << (character.rate * 100) << "%" << std::endl;
                }
                
                // Tampilkan SR
                auto srChars = gachaSystem.getCharactersByRarity("SR");
                setConsoleColor(MAGENTA);
                std::cout << "\n[ SR Characters (Rate: " << (gachaSystem.getRarityRate("SR") * 100) << "%) ]\n";
                resetConsoleColor();
                
                std::cout << std::left << std::setw(15) << "Nama" << std::setw(20) << "Title" 
                        << std::setw(10) << "Element" << "Rate" << std::endl;
                std::cout << std::string(60, '-') << std::endl;
                
                for (const auto& character : srChars) {
                    std::cout << std::left << std::setw(15) << character.name 
                            << std::setw(20) << character.title 
                            << std::setw(10) << character.element
                            << (character.rate * 100) << "%" << std::endl;
                }
                
                // Tampilkan R
                auto rChars = gachaSystem.getCharactersByRarity("R");
                setConsoleColor(CYAN);
                std::cout << "\n[ R Characters (Rate: " << (gachaSystem.getRarityRate("R") * 100) << "%) ]\n";
                resetConsoleColor();
                
                std::cout << std::left << std::setw(15) << "Nama" << std::setw(20) << "Title" 
                        << std::setw(10) << "Element" << "Rate" << std::endl;
                std::cout << std::string(60, '-') << std::endl;
                
                for (const auto& character : rChars) {
                    std::cout << std::left << std::setw(15) << character.name 
                            << std::setw(20) << character.title 
                            << std::setw(10) << character.element
                            << (character.rate * 100) << "%" << std::endl;
                }
                
                break;
            }
            case 7: {
                // Lihat Info Rate
                std::cout << "Informasi Rate Gacha:\n\n";
                
                setConsoleColor(YELLOW);
                double ssrRate = gachaSystem.getCurrentSSRRate();
                std::cout << "SSR Rate: " << (ssrRate * 100) << "%" << std::endl;
                resetConsoleColor();
                
                if (gachaSystem.isInSoftPity()) {
                    std::cout << "  ↳ Anda dalam kondisi soft pity! Rate SSR telah meningkat!" << std::endl;
                }
                
                setConsoleColor(MAGENTA);
                std::cout << "SR Rate: " << (gachaSystem.getRarityRate("SR") * 100) << "%" << std::endl;
                resetConsoleColor();
                
                setConsoleColor(CYAN);
                std::cout << "R Rate: " << (gachaSystem.getRarityRate("R") * 100) << "%" << std::endl;
                resetConsoleColor();
                
                std::cout << "Common Rate: " << (gachaSystem.getRarityRate("Common") * 100) << "%" << std::endl;
                
                std::cout << "\nInformasi Pity:\n";
                std::cout << "- Hard Pity: Dijamin mendapatkan SSR pada pull ke-90\n";
                std::cout << "- Soft Pity: Rate SSR meningkat 5x setelah pull ke-75\n";
                
                std::cout << "\nStatus Pity Counter Anda:\n";
                std::cout << "Pull tersisa sampai hard pity";

                std::cout << "Pull tersisa sampai hard pity: " << gachaSystem.getPityCounter() << std::endl;
                
                int softPityCounter = gachaSystem.getSoftPityCounter();
                if (softPityCounter <= 0) {
                    std::cout << "Anda dalam kondisi soft pity! Rate SSR telah meningkat!" << std::endl;
                } else {
                    std::cout << "Pull tersisa sampai soft pity: " << softPityCounter << std::endl;
                }
                
                std::cout << "Karakter pity saat ini: " << gachaSystem.getSelectedPityCharName() << std::endl;
                break;
            }
            case 8: {
                // Simulasi Gacha (Sampai dapat SSR)
                std::cout << "Simulasi Gacha (Sampai dapat SSR):\n\n";
                
                int pullsNeeded = 0;
                bool gotSSR = false;
                
                while (!gotSSR) {
                    GachaResult result = gachaSystem.pull();
                    pullsNeeded++;
                    
                    if (result.rarity == "SSR") {
                        gotSSR = true;
                        std::cout << "SSR didapatkan pada pull ke-" << pullsNeeded << "!\n";
                        GachaSystem::printResult(result, gachaSystem.getAllCharacters());
                    }
                    
                    // Tampilkan progres setiap 10 pull
                    if (pullsNeeded % 10 == 0 && !gotSSR) {
                        std::cout << "Sudah melakukan " << pullsNeeded << " pull, belum mendapatkan SSR...\n";
                    }
                    
                    // Batasi simulasi untuk mencegah loop terlalu lama
                    if (pullsNeeded >= 100 && !gotSSR) {
                        std::cout << "Simulasi dihentikan setelah 100 pull.\n";
                        break;
                    }
                }
                
                std::cout << "\nTotal currency yang digunakan: " << (pullsNeeded * 160) << " (jika 1 pull = 160 currency)\n";
                break;
            }
            case 0:
                std::cout << "Terima kasih telah menggunakan sistem gacha!\n";
                break;
            default:
                std::cout << "Pilihan tidak valid.\n";
                break;
        }
        
        if (choice != 0) {
            std::cout << "\nTekan Enter untuk kembali ke menu...";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cin.get();
        }
        
    } while (choice != 0);
    
    return 0;
}