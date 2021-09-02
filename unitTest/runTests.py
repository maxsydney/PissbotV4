import os
import sys
from pathlib import Path
import unity_test_parser
import junit_xml

RAW_RESULTS_PATH = "outputs/testResults.txt"
PROCESSED_RESULTS_PATH = "outputs/test_results.xml"

def main():
    print("\n\n\tPB Unit Test Runner\n\n")

    # Check if required variables are set
    qemuPath = os.getenv("QEMU_PATH")
    if not qemuPath:
        print("Environment variable QEMU_PATH was not set")
        sys.exit(os.EX_USAGE)

    PBPath = os.getenv("PBPATH")
    if not PBPath:
        print("Environment variable PBPATH was not set")
        sys.exit(os.EX_USAGE)

    # Switch to unitTest directory
    os.chdir(f"{PBPath}/unitTest")

    # If outputs directory doesn't exist, create it
    Path("outputs").mkdir(parents=True, exist_ok=True)

    # Run tests
    print("Building tests")
    os.system("idf.py build")

    print("\n\tGenerating test binary\n")
    os.system("./makeUnitTestImg.sh")

    print("\n\tLaunching Qemu and running tests\n")
    os.system(f"{qemuPath}/qemu-system-xtensa -nographic "
              "--no-reboot -machine esp32 -drive file=outputs/unitTest.bin,if=mtd,format=raw "
              f"-serial file:{RAW_RESULTS_PATH}")

    print("\n\tTests complete. Processing results\n")
    with open(RAW_RESULTS_PATH, "r") as results_file:
        try:
            results = unity_test_parser.TestResults(results_file.read())
            for test in results.test_iter():
                print(f"Test: {test.name()}: {test.result()}")
            with open(PROCESSED_RESULTS_PATH, "w") as out_file:
                junit_xml.TestSuite.to_file(out_file, [results.to_junit()])
            print(f"\n\tProcessing complete. Saving processed results to {PROCESSED_RESULTS_PATH}")
            sys.exit(os.EX_OK)
        except Exception as e:
            print("\n\tTest processing failed")
            print(e)
            sys.exit(os.EX_DATAERR)

if __name__ == "__main__":
    main()
