import os
import re
import sys

def lint_file(file_path):
    issues = []
    with open(file_path, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    filename = os.path.basename(file_path)
    
    # 1. Check filename (snake_case)
    if not re.match(r'^[a-z0-9_]+\.(h|cpp|hpp)$', filename):
        issues.append(f"Filename '{filename}' should be snake_case.")

    is_header = file_path.endswith('.h') or file_path.endswith('.hpp')
    
    in_class = False
    in_public = False
    
    for i, line in enumerate(lines):
        line_num = i + 1
        stripped = line.strip()
        
        # Skip empty lines and comments for most checks
        if not stripped or stripped.startswith('//') or stripped.startswith('*') or stripped.startswith('/*'):
            continue

        # 2. Check Class/Struct naming (PascalCase)
        # Avoid matching words in comments or other contexts
        class_match = re.search(r'^\s*(class|struct)\s+([a-z][a-zA-Z0-9]*)\b', stripped)
        if class_match:
             issues.append(f"L{line_num}: Class/Struct '{class_match.group(2)}' should be PascalCase.")
        
        if 'class' in stripped or 'struct' in stripped:
            in_class = True
            in_public = False # Classes default to private
            if 'struct' in stripped: in_public = True # Structs default to public
        
        if stripped.startswith('public:'): in_public = True
        if stripped.startswith('private:') or stripped.startswith('protected:'): in_public = False
        
        # 3. Check Method naming (camelCase) and Doxygen in headers
        if is_header and in_class and in_public:
            # Match method declarations: type name(args)
            # Filter out constructors/destructors and common false positives
            method_match = re.search(r'^\s*(?:virtual\s+)?(?:[a-zA-Z0-9_<>]+\s+)?([a-z][a-zA-Z0-9]*)\s*\(', stripped)
            if method_match:
                method_name = method_match.group(1)
                if method_name not in ['if', 'while', 'for', 'switch', 'return', 'using']:
                    # Check for Doxygen comment above
                    has_doxygen = False
                    for j in range(i-1, max(0, i-10), -1):
                        prev_line = lines[j].strip()
                        if not prev_line: continue
                        if '*/' in prev_line or '///' in prev_line or '/**' in prev_line:
                            has_doxygen = True
                            break
                        if ';' in prev_line or '}' in prev_line or '{' in prev_line:
                            break
                    if not has_doxygen:
                        issues.append(f"L{line_num}: Public method '{method_name}' is missing Doxygen documentation.")

        # 4. Check Private member naming (snake_case_)
        if in_class and not in_public:
            # Match member variables: type name_; or type name;
            member_match = re.search(r'^\s*(?:const\s+)?(?:static\s+)?(?:[a-zA-Z0-9_<>]+\s+)+([a-z][a-z0-9_]*)(?<!_);\s*$', stripped)
            if member_match:
                member_name = member_match.group(1)
                if member_name not in ['public', 'private', 'protected']:
                    issues.append(f"L{line_num}: Private member '{member_name}' should be snake_case_ (ending with underscore).")

    return issues

def main():
    src_dir = 'src'
    all_issues = {}
    
    for root, dirs, files in os.walk(src_dir):
        for file in files:
            if file.endswith(('.h', '.cpp', '.hpp')):
                path = os.path.join(root, file)
                issues = lint_file(path)
                if issues:
                    all_issues[path] = issues
                    
    if all_issues:
        print("Style violations found:")
        for path, issues in all_issues.items():
            print(f"\nFile: {path}")
            for issue in issues:
                print(f"  - {issue}")
        sys.exit(1)
    else:
        print("No style violations found.")
        sys.exit(0)

if __name__ == "__main__":
    main()
