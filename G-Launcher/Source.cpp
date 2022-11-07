#include <iostream>
#include <string>
#include <stdio.h>
#include <codecvt>
#include <filesystem>
#include <curl/curl.h>
#include <json/json.h>
#include <fstream>
#include <urlmon.h>
#pragma comment(lib,"urlmon.lib")

namespace fs = std::filesystem;

const std::string version_current = "v1.1";

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::wstring ConvertUtf8ToWide(const std::string& str)
{
    int count = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), NULL, 0);
    std::wstring wstr(count, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), &wstr[0], count);
    return wstr;
}

std::string GetLatestVersion()
{
    // Setup cURL
    const std::string url = "https://api.github.com/repos/MichaelSenescall/G-Launcher/tags";
    long resCode = 0;
    std::string readBuffer;

    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    // Headers
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: G-Launcher");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Run cURL
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resCode);
    curl_easy_cleanup(curl);
    if (resCode != 200) return "failed";

    // Parse JSON to get latest version
    Json::Value jsonData;
    Json::Reader jsonReader;
    if (!jsonReader.parse(readBuffer.c_str(), jsonData)) return "failed";

    std::string version_latest = jsonData[0]["name"].asString();

    return version_latest;
}

bool UpdateNeeded()
{
    // Get latest version
    std::string version_latest = GetLatestVersion();
    if (version_latest == "failed") return false;

    // Convert to float
    float num_version_current = std::stof(version_current.substr(1, version_current.size() - 1));
    float num_version_latest = std::stof(version_latest.substr(1, version_latest.size() - 1));

    // Final check
    if (num_version_current < num_version_latest)
        return true;
    else
        return false;
}

void DownloadLatestFile(std::string cur_pathwfile, std::string out_pathwfile)
{
    // Get latest version
    std::string version_latest = GetLatestVersion();
    if (version_latest == "failed") return;

    // Download latest version
    std::wstring download_url = L"https://github.com/MichaelSenescall/G-Launcher/releases/latest/download/G-Launcher.exe";
    std::wstring download_pathwfile = ConvertUtf8ToWide(out_pathwfile);

    HRESULT hr = URLDownloadToFile(NULL, download_url.c_str(), download_pathwfile.c_str(), 0, NULL);

    if (hr != S_OK) return;

    // Rename old and new files
    std::string renamed = cur_pathwfile + ".G-Launcher-DELETE";
    rename(cur_pathwfile.c_str(), renamed.c_str());
    rename(out_pathwfile.c_str(), cur_pathwfile.c_str());
    remove(renamed.c_str());
}

int main(int argc, char** argv) {
    // Setup local variables
    std::string cur_pathwfile = argv[0];
    std::string cur_path = cur_pathwfile.substr(0, cur_pathwfile.find_last_of("/\\"));
    std::string cur_filename = cur_pathwfile.substr(cur_pathwfile.find_last_of("/\\") + 1);
    
    std::string out_filename = "G-Launcher.exe.new";
    std::string out_pathwfile = cur_path + "\\" + out_filename;

    // Delete any old files
    for (const auto& file : fs::recursive_directory_iterator(cur_path))
    {
        std::string path = fs::absolute(file.path()).string();
        remove(path.c_str());
    }
        
    // Check for updates
    if (UpdateNeeded()) DownloadLatestFile(cur_pathwfile, out_pathwfile);

	return 0;
}