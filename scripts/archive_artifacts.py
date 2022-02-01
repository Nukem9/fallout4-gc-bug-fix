import argparse
import os
import zipfile

def make_rel_archive(a_args):
	archive = zipfile.ZipFile(f"{a_args.name}.zip", "w", zipfile.ZIP_DEFLATED)
	# Archive dll
	archive.write(a_args.dll, f"F4SE/Plugins/{os.path.basename(a_args.dll)}")
	# Archive ini
	iniPath = os.path.join(a_args.srcpath, "res", f"{a_args.name}.ini")
	archive.write(iniPath, f"F4SE/Plugins/{os.path.basename(iniPath)}")

def parse_arguments():
	parser = argparse.ArgumentParser(description="archive build artifacts for distribution")
	parser.add_argument("--dll", type=str, help="the full dll path", required=True)
	parser.add_argument("--name", type=str, help="the project name", required=True)
	parser.add_argument("--srcpath", type=str, help="source code root directory", required=True)
	return parser.parse_args()

def main():
	out = "artifacts"
	try:
		os.mkdir(out)
	except FileExistsError:
		pass
	os.chdir(out)

	args = parse_arguments()
	make_rel_archive(args)

if __name__ == "__main__":
	main()
