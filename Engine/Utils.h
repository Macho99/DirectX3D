#pragma once

class Utils
{
public:
	static bool StartsWith(string str, string comp);
	static bool StartsWith(wstring str, wstring comp);

	static void Replace(OUT string& str, string cmp, string rep);
	static void Replace(OUT wstring& str, wstring cmp, wstring rep);

	static wstring ToWString(string value);
	static string ToString(wstring value);

	static string ToString(Vec3& value);	
	
	static ComPtr<ID3D11ShaderResourceView> CreateTexture2DArraySRV(vector<wstring>& filenames);

	static vector<Vec4> ParseUVText(const wstring& path);
};

