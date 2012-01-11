using System;
using System.IO;
using System.Runtime.Serialization.Formatters.Binary;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.Drawing;
using System.Drawing.Imaging;

namespace cv2conv
{
    class Program
    {
        const string programName = "cv2conv";

        struct ProgramOptions
        {
            public int Color;
            public bool Recursive;
            public ImageFormat ImageFormat;
            public string ImageExtension;
        }

        struct ProgramParameters
        {
            public ProgramOptions Options;
            public List<string> FileNames;
        }

        static ProgramParameters GetProgramParameters(string[] args)
        {
            Option[] options = {
                    new Option("-R", false),
                    new Option("COLOR", true),
                    new Option("FORMAT", true),
            };

            Dictionary<string, ImageFormat> imageFormatTable
                = new Dictionary<string, ImageFormat>();
            imageFormatTable.Add("BMP", ImageFormat.Bmp);
            imageFormatTable.Add("PNG", ImageFormat.Png);
            imageFormatTable.Add("JPEG", ImageFormat.Jpeg);

            ProgramParameters param = new ProgramParameters();
            param.Options = new ProgramOptions();
            param.Options.Recursive = true;
            param.Options.Color = 1;
            param.Options.ImageExtension = "PNG";
            OptionResult result = OptionParser.Parse(options, args);

            if (result.Options.ContainsKey("-R"))
                param.Options.Recursive = false;
            if (result.Options.ContainsKey("COLOR"))
            {
                param.Options.Color = Int32.Parse(result.Options["COLOR"]);
                if (param.Options.Color < 1 || param.Options.Color > 3)
                    throw new FormatException("カラーは1から3の間を指定してください。");
            }
            if (result.Options.ContainsKey("FORMAT"))
            {
                param.Options.ImageExtension = result.Options["FORMAT"];
                if (!imageFormatTable.ContainsKey(param.Options.ImageExtension))
                    throw new FormatException("無効なファイル形式が指定されました。");
            }
            param.Options.ImageFormat = imageFormatTable[param.Options.ImageExtension];
            if (result.Rest.Count == 0)
                throw new FormatException("パスが指定されていません。");
            param.FileNames = result.Rest;

            return param;
        }

        static Color[] LoadPalette(string paletteName)
        {
            Dictionary<int, CCDelegate> paletteCCTable
                = new Dictionary<int, CCDelegate>();
            paletteCCTable.Add(16, new CCDelegate(ColourConverter.FromBGRA5551));
            paletteCCTable.Add(24, new CCDelegate(ColourConverter.FromBGR888));
            paletteCCTable.Add(32, new CCDelegate(ColourConverter.FromBGRA8888));

            using (FileStream pal = new FileStream(paletteName, FileMode.Open, FileAccess.Read))
            {
                if (pal.Length < 1)
                    throw new IOException("パレットサイズが小さすぎます。");

                int bpp = pal.ReadByte();
                if (pal.Length != (bpp >> 3) * 256 + 1)
                    throw new IOException("パレットサイズが小さすぎます。");
                if (!paletteCCTable.ContainsKey(bpp))
                    throw new FormatException("サポートしていないパレットです");

                Color[] palette = new Color[256];
                CCDelegate cc = paletteCCTable[bpp];
                byte[] palBuf = new byte[bpp >> 3];
                for (int i = 0; i < 256; ++i)
                {
                    pal.Read(palBuf, 0, palBuf.Length);
                    palette[i] = cc(palBuf);
                }

                return palette;
            }
        }

        static void ProcessCV2(string fileName, ProgramOptions options)
        {
            try
            {
                System.Console.WriteLine(fileName);

                using (FileStream fs = new FileStream(fileName, FileMode.Open, FileAccess.Read))
                {
                    CV2Reader cv2 = new CV2Reader(fs);

                    Bitmap image = cv2.GetImage();

                    if (cv2.Header.bpp == 8)
                    {
                        string paletteName =
                            Path.Combine(Path.GetDirectoryName(fileName),
                                String.Format("palette{0:D3}.pal", options.Color - 1));

                        Color[] newPalette = LoadPalette(paletteName);
                        ColorPalette palette = image.Palette;
                        Array.Copy(newPalette, palette.Entries, 256);
                        image.Palette = palette;
                    }
                    string outputName =
                        Path.ChangeExtension(fileName, options.ImageExtension.ToLower());
                    image.Save(outputName, options.ImageFormat);
                }
            }
            catch (Exception e)
            {
                System.Console.Error.Write(programName + " : " + e.Message);
            }
        }

        static void ProcessCV2Directory(string dirName, ProgramOptions options)
        {
            SearchOption searchOpt = options.Recursive ?
                  SearchOption.AllDirectories
                : SearchOption.TopDirectoryOnly;
            string[] files = Directory.GetFiles(dirName, "*.CV2", searchOpt);
            Array.Sort(files);
            foreach (string fileName in files)
            {
                ProcessCV2(fileName, options);
            }
        }

        static void Main(string[] args)
        {
            if (args.Length < 1)
            {
                System.Console.WriteLine("コマンドラインの書式：");
                System.Console.WriteLine(programName + " [/-R] [/COLOR[:]1-3] [/FORMAT[:]出力形式名] [ドライブ:][パス]ファイル名");
                System.Console.WriteLine("  /-R     ディレクトリを指定したとき、サブディレクトリをスキャンしません。");
                System.Console.WriteLine("  /COLOR[:]1-3");
                System.Console.WriteLine("          可能なとき、指定したインデックスのパレットで出力します。既定値は1です。");
                System.Console.WriteLine("  /FORMAT[:]出力形式");
                System.Console.WriteLine("          出力ファイルの形式を指定します。");
                System.Console.WriteLine("          形式には BMP, JPEG, PNG を指定することが出来ます。");
                return;
            }
            try
            {
                ProgramParameters param = GetProgramParameters(args);

                foreach (string fileName in param.FileNames)
                {
                    FileAttributes attr = File.GetAttributes(fileName);
                    if ((attr & FileAttributes.Directory) != 0)
                    {
                        ProcessCV2Directory(fileName, param.Options);
                    }
                    else
                    {
                        ProcessCV2(fileName, param.Options);
                    }
                }
            }
            catch (Exception e)
            {
                System.Console.Error.Write(programName + " : " + e.Message);
            }
        }
    }

    // ここからCV2リーダ
    public class CV2Reader
    {
        private Stream cv2;
        private CV2Header _header;
        public CV2Header Header
        {
            get { return _header; }
        }

        public CV2Reader(Stream input)
        {
            if (input.Length < 17)
                throw new IOException("ファイルが小さすぎます");
            _header = CV2Header.Deserialize(input);
            if (_header.Height * _header.PaddedWidth * _header.Bpp > input.Length - 17)
                throw new IOException("ファイルが小さすぎます");
            cv2 = input;
        }

        public Bitmap GetImage()
        {
            cv2.Seek(17, SeekOrigin.Begin);

            Bitmap image = new Bitmap(_header.Width, _header.Height, _header.PixelFormat);

            int lineSize = _header.Width * _header.Bpp;
            byte[] lineBuf = new byte[_header.PaddedWidth * _header.Bpp];
            for (int y = 0; y < _header.Height; ++y)
            {
                cv2.Read(lineBuf, 0, lineBuf.Length);
                BitmapData data = image.LockBits(
                     new Rectangle(0, y, _header.Width, 1),
                     ImageLockMode.WriteOnly, _header.PixelFormat);
                Marshal.Copy(lineBuf, 0, data.Scan0, lineSize);
                image.UnlockBits(data);
            }
            return image;
        }
    }

    public class CV2Header
    {
        private static Dictionary<int, PixelFormat> pixelFormatTable
            = new Dictionary<int, PixelFormat>();
        private static Dictionary<int, int> bppTable
            = new Dictionary<int, int>();

        static CV2Header()
        {
            pixelFormatTable.Add(8, PixelFormat.Format8bppIndexed);
            pixelFormatTable.Add(16, PixelFormat.Format16bppRgb565);
            pixelFormatTable.Add(24, PixelFormat.Format32bppArgb);
            pixelFormatTable.Add(32, PixelFormat.Format32bppArgb);

            bppTable.Add(8, 1);
            bppTable.Add(16, 2);
            bppTable.Add(24, 4); // はぁ？
            bppTable.Add(32, 4);
        }

        private int _bpp;
        public int bpp
        {
            get { return _bpp; }
            set
            {
                if (!pixelFormatTable.ContainsKey(value))
                    throw new FormatException("サポートしていない色深度です。");
                _bpp = value;
                _pixelFormat = pixelFormatTable[value];
            }
        }

        public int Bpp
        {
            get { return bppTable[_bpp]; }
        }

        private PixelFormat _pixelFormat;
        public PixelFormat PixelFormat
        {
            get { return _pixelFormat; }
        }

        int _width;
        public int Width
        {
            get { return _width; }
            set { _width = value; }
        }

        int _height;
        public int Height
        {
            get { return _height; }
            set { _height = value; }
        }

        int _pwidth;
        public int PaddedWidth
        {
            get { return _pwidth; }
            set { _pwidth = value; }
        }


        public static CV2Header Deserialize(Stream input)
        {
            CV2Header header = new CV2Header();
            BinaryReaderEx reader = new BinaryReaderEx(input);
            header.bpp = reader.ReadByte();
            header.Width = reader.ReadInt32();
            header.Height = reader.ReadInt32();
            header.PaddedWidth = reader.ReadInt32();
            int resd1 = reader.ReadInt32();

            if (header.PaddedWidth < header.Width)
                throw new FormatException("ヘッダーが不正です。");
            return header;
        }
    }

    // ここからカラーコンバータ
    delegate Color CCDelegate(byte[] src);

    class ColourConverter
    {
        // 高速化？
        static readonly int[] Color5Bits = {
            0, 8, 16, 24, 32, 41, 49, 57, 
            65, 74, 82, 90, 98, 106, 115, 123, 
            131, 139, 148, 156, 164, 172, 180, 189, 
            197, 205, 213, 222, 230, 238, 246, 255
        };
        static readonly int[] Color6Bits = {
            0, 4, 8, 12, 16, 20, 24, 28, 
            32, 36, 40, 44, 48, 52, 56, 60, 
            64, 68, 72, 76, 80, 85, 89, 93, 
            97, 101, 105, 109, 113, 117, 121, 125, 
            129, 133, 137, 141, 145, 149, 153, 157, 
            161, 165, 170, 174, 178, 182, 186, 190, 
            194, 198, 202, 206, 210, 214, 218, 222, 
            226, 230, 234, 238, 242, 246, 250, 255
        };

        public static Color FromBGRA5551(byte[] src)
        {
            int A = (src[1] & 0x80) > 0 ? 0xFF : 0x00;
            int R = (src[1] & 0x7C) >> 2;
            int G = ((src[1] & 0x03) << 3) | ((src[0] & 0xE0) >> 5);
            int B = src[0] & 0x1F;
            return Color.FromArgb(A, Color5Bits[R], Color5Bits[G], Color5Bits[B]);
        }

        public static Color FromBGR565(byte[] src)
        {
            int R = src[1] & 0xF8 >> 3;
            int G = ((src[1] & 0x07) << 3) | ((src[0] & 0xE0) >> 5);
            int B = src[0] & 0x1F;
            return Color.FromArgb(Color5Bits[R], Color6Bits[G], Color5Bits[B]);
        }

        public static Color FromBGR888(byte[] src)
        {
            return Color.FromArgb(src[2], src[1], src[0]);
        }

        public static Color FromBGRA8888(byte[] src)
        {
            return Color.FromArgb(src[3], src[0], src[1], src[2]);
        }
    }

    // ここからオプションパーザ
    struct Option
    {
        public Option(string Name, bool HasValue)
        {
            this.Name = Name; this.HasValue = HasValue;
        }

        public string Name;
        public bool HasValue;
    }

    struct OptionResult
    {
        public Dictionary<string, string> Options;
        public List<string> Rest;
    }

    class OptionParser
    {
        enum MatchOpentionResult
        {
            Found,
            NotFound,
            NeedsValue,
            NotNeedsValue,
        }

        static MatchOpentionResult MatchOption(Option[] options, string Name, bool hasValue)
        {
            for (int j = 0; j < options.Length; ++j)
            {
                if (options[j].Name == Name)
                {
                    if (options[j].HasValue == hasValue)
                        return MatchOpentionResult.Found;
                    else if (options[j].HasValue && !hasValue)
                        return MatchOpentionResult.NeedsValue;
                    else
                        return MatchOpentionResult.NotNeedsValue;
                }
            }
            return MatchOpentionResult.NotFound;
        }

        enum ParseState
        {
            FetchName,
            FetchValue,
        }

        public static OptionResult Parse(Option[] options, string[] args)
        {
            OptionResult result;
            result.Options = new Dictionary<string, string>();
            result.Rest = new List<string>();

            ParseState state = ParseState.FetchName;
            string Name = null;
            bool hasValue;

            Regex Px = new Regex("^/(?<Name>[\\w\\d@\\+\\-]+)(?::(?<Value>.*))?$", RegexOptions.IgnoreCase);

            foreach (string arg in args)
            {
                Match match = Px.Match(arg);
                switch (state)
                {
                    case ParseState.FetchName:
                        if (match.Success)
                        {
                            Name = match.Groups["Name"].Value;
                            hasValue = match.Groups["Value"].Captures.Count > 0;
                            switch (MatchOption(options, Name, hasValue))
                            {
                                case MatchOpentionResult.Found:
                                    result.Options[Name] = match.Groups["Value"].Value;
                                    break;
                                case MatchOpentionResult.NeedsValue:
                                    state = ParseState.FetchValue;
                                    break;
                                default:
                                    throw new FormatException("無効なオプションが指定されました : /" + Name);
                            }
                        }
                        else
                            result.Rest.Add(arg);
                        break;
                    case ParseState.FetchValue:
                        if (match.Success)
                            throw new FormatException("オプションに値が指定されていません : /" + Name);
                        else
                        {
                            result.Options[Name] = arg;
                            state = ParseState.FetchName;
                        }
                        break;
                }
            }
            if (state == ParseState.FetchValue)
                throw new FormatException("オプションに値が指定されていません : /" + Name);
            return result;
        }
    }

    // ここからバイナリリーダ
    enum Endian
    {
        Big,
        Little
    }

    class BinaryReaderEx
    {
        private Endian machineEndian =
            BitConverter.IsLittleEndian
                ? Endian.Little
                : Endian.Big;
        public Stream BaseStream = null;
        public Endian endian = Endian.Little;

        public BinaryReaderEx(Stream input)
        {
            BaseStream = input;
        }

        public void ReadBytes(byte[] buff, int offset, int count)
        {
            BaseStream.Read(buff, offset, count);
        }

        public byte[] ReadBytes(int count)
        {
            byte[] buff = new byte[count];
            BaseStream.Read(buff, 0, count);
            return buff;
        }

        public int ReadByte()
        {
            return BaseStream.ReadByte();
        }

        public short ReadInt16()
        {
            return BitConverter.ToInt16(ReadBytesInternal(2), 0);
        }

        public int ReadInt32()
        {
            return BitConverter.ToInt32(ReadBytesInternal(4), 0);
        }

        public ushort ReadUInt16()
        {
            return BitConverter.ToUInt16(ReadBytesInternal(2), 0);
        }

        public uint ReadUInt32()
        {
            return BitConverter.ToUInt32(ReadBytesInternal(4), 0);
        }

        public float ReadSingle()
        {
            return BitConverter.ToSingle(ReadBytesInternal(4), 0);
        }

        private byte[] ReadBytesInternal(int count)
        {
            byte[] buff = ReadBytes(count);
            if (machineEndian != endian)
            {
                Array.Reverse(buff);
            }
            return buff;
        }
    }
}
