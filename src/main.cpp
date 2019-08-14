#include <algorithm>
#include <iostream>
#include <lodepng.h>

using namespace std;

struct Color
{
	uint8_t r;
	uint8_t g;
	uint8_t b;

	Color(uint8_t _r, uint8_t _g, uint8_t _b)
		: r(_r)
		, g(_g)
		, b(_b)
	{

	}
};

struct Image
{
	string filename;
	uint32_t width = 0;
	uint32_t height = 0;
	vector<uint8_t> bitmap;
};

Color *background;
Color *foreground;

Color *output_color = foreground;

Image input;
Image output;

void ShowHelp()
{
	cout <<
		"Usage: transparency -b hex -f hex -i filename -o filename [-c hex]\n"
		"\n"
		"Options:\n"
		"  --help       show this help\n"
		"\n"
		"  -b hex       background color\n"
		"  -f hex       foreground color\n"
		"  -i filename  input image\n"
		"  -o filename  output image\n"
		"  -c hex       output color; default = <foreground color>" << endl;
}

bool ReadHEX(Color *&color, char *str)
{
	if (str == nullptr) return true;

	if (strlen(str) != 6)
	{
		cout << "Error: ReadHEX(): \"" << str << "\" is not a 6-digit hex color" << endl;

		return true;
	}
	
	char *end;

	uint32_t number = strtoul(str, &end, 16);

	if (*end != '\0')
	{
		cout << "Error: ReadHEX(): \"" << str << "\" is not a valid hex color" << endl;

		return true;
	}

	color = new Color(number >> 16, number >> 8, number);

	return false;
}

bool ReadString(string &string, char *str)
{
	if (str == nullptr) return true;

	string = str;

	return false;
}

char *Argument(int argc, char *argv[], int &i)
{
	i++;

	if (argc == i)
	{
		cout << "Error: The option " << argv[i - 1] << " requires an argument" << endl;

		return nullptr;
	}

	return argv[i];
}

bool Options(int argc, char *argv[])
{
	char *str;

	for (int i = 0; i < argc; i++)
	{
		str = argv[i];

		if (strlen(str) == 2)
		{
			uint16_t option = str[0] << 8 | str[1];

			switch (option)
			{
			case '-b':
				if (ReadHEX(background, Argument(argc, argv, i))) return true;

				break;
			case '-f':
				if (ReadHEX(foreground, Argument(argc, argv, i))) return true;

				break;
			case '-i':
				if (ReadString(input.filename, Argument(argc, argv, i))) return true;

				break;
			case '-o':
				if (ReadString(output.filename, Argument(argc, argv, i))) return true;

				break;
			case '-c':
				if (ReadHEX(output_color, Argument(argc, argv, i))) return true;
				
				break;
			}
		}
		else if (strcmp(str, "--help") == 0)
		{
			ShowHelp();

			return true;
		}
	}

	return false;
}

bool Decode(Image &image)
{
	uint32_t error = lodepng::decode(image.bitmap, image.width, image.height, image.filename);

	if (error)
	{
		std::cout << "Error #" << error << ": decode(" << image.filename << "): " << lodepng_error_text(error) << std::endl;

		return true;
	}

	return false;
}

bool Encode(Image &image)
{
	uint32_t error = lodepng::encode(image.filename, image.bitmap, image.width, image.height);

	if (error)
	{
		std::cout << "Error #" << error <<": encode(" << image.filename << "): " << lodepng_error_text(error) << std::endl;

		return true;
	}

	return false;
}

bool Open() 
{
	if (Decode(input)) return true;

	output.width = input.width;
	output.height = input.height;
	output.bitmap.resize(output.width * output.height * 4);

	return false;
}

uint8_t Alpha(uint8_t background, uint8_t foreground, uint8_t input)
{
	if (foreground == background)
		return 255;

	return 255 * (input - background) / (foreground - background); // a = (r - b) / (f - b)
}

uint8_t MaxAlpha(uint8_t *input)
{
	uint8_t alpha = Alpha(background->r, foreground->r, input[0]);

	alpha = max(alpha, Alpha(background->g, foreground->g, input[1]));
	alpha = max(alpha, Alpha(background->b, foreground->b, input[2]));

	return alpha;
}

bool Process()
{
	uint8_t *pixel;

	for (uint32_t i = 0; i < output.width * output.height * 4; i += 4)
	{
		pixel = &output.bitmap[i];
		
		pixel[0] = output_color->r;
		pixel[1] = output_color->g;
		pixel[2] = output_color->b;

		pixel[3] = MaxAlpha(&input.bitmap[i]);
	}

	return false;
}

bool Close()
{
	if (Encode(output)) return true;

	cout << "Output: " << output.filename << endl;

	return false;
}

int main(int argc, char *argv[])
{
	if (Options(argc, argv)) return -1;

	//
	if (background == nullptr) return -1;
	if (foreground == nullptr) return -1;

	if (output_color == nullptr)
		output_color = foreground;
	
	//
	if (Open()) return -1;
	if (Process()) return -1;
	if (Close()) return -1;

	return 0;
}
