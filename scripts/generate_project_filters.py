#!/usr/bin/env python3

import argparse
from pathlib import Path
import re

import call_wrapper

PROJECT_PREFIX = '''<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">\n'''

PROJECT_POSTFIX = '</Project>\n'

def generate(project_file):
    project_file = Path(project_file)

    if not project_file.exists():
        raise ValueError(f'Project file path is invalid: {project_file}')

    output_file = project_file.parent/f'{project_file.name}.filters'
    output_file.unlink(missing_ok=True)

    with open(project_file, 'r') as f:
        project_data = f.read()

    # collect include patterns from project file
    source_file_patterns = []
    for include_prefix, pattern in re.findall('<Cl(Compile|Include) Include="(.+)" />', project_data):
        prefix = '$(MsbuildThisFileDirectory)'
        source_file_patterns.append({
            'include_prefix': include_prefix,
            'path_prefix': prefix if pattern.startswith(prefix) else '',
            'include_pattern': pattern.removeprefix(prefix)
        })

    # glob include patterns
    source_files = []
    for src in source_file_patterns:
        source_files += [
            {
                'include_prefix': src['include_prefix'],
                'path_prefix': src['path_prefix'],
                'path': i.relative_to(output_file.parent)
            } for i in list(project_file.parent.glob(src['include_pattern']))
        ]

    # calculate file filters
    for src in source_files:
        filter = str(src['path'].parent)
        while filter.startswith('..\\'):
            filter = filter.removeprefix('..\\')
        src['filter'] = filter if filter and filter != '.' else None

    # split files by filters
    source_files_groups = []
    for include_prefix in {i['include_prefix'] for i in source_files}:
        source_files_groups.append([i for i in source_files if i['include_prefix'] == include_prefix])

    with open(output_file, 'w') as output:
        output.write(PROJECT_PREFIX)
        for source_files_group in source_files_groups:
            output.write('  <ItemGroup>\n')
            for src in source_files_group:
                include_prefix = src['include_prefix']
                filter = src['filter']
                path = src['path']
                path_prefix = src['path_prefix']
                output.write(f'    <Cl{include_prefix} Include="{path_prefix}{path}">\n')
                if filter:
                    output.write(f'        <Filter>{filter}</Filter>\n')
                output.write(f'    </Cl{include_prefix}>\n')
            output.write('  </ItemGroup>\n')
        output.write('  <ItemGroup>\n')
        for i in {i['filter'] for i in source_files if i['filter']}:
            output.write(f'    <Filter Include="{i}"/>\n')
        output.write('  </ItemGroup>\n')
        output.write(PROJECT_POSTFIX)

    print(f"Generated file: {output_file}")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Generate project filter file')
    parser.add_argument('--project_file')

    args = parser.parse_args()

    call_wrapper.final_call_decorator(
        "Generating project filter file",
        "Generating: success",
        "Generating: failure!"
    )(
        generate
    )(
        args.project_file
    )
