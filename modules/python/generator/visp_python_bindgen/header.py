#############################################################################
#
# ViSP, open source Visual Servoing Platform software.
# Copyright (C) 2005 - 2023 by Inria. All rights reserved.
#
# This software is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# See the file LICENSE.txt at the root directory of this source
# distribution for additional information about the GNU GPL.
#
# For using ViSP with software that can not be combined with the GNU
# GPL, please contact Inria about acquiring a ViSP Professional
# Edition License.
#
# See https://visp.inria.fr for more information.
#
# This software was developed at:
# Inria Rennes - Bretagne Atlantique
# Campus Universitaire de Beaulieu
# 35042 Rennes Cedex
# France
#
# If you have questions regarding the use of this file, please contact
# Inria at visp@inria.fr
#
# This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
# WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
# Description:
# ViSP Python bindings generator
#
#############################################################################


from typing import List, Optional, Dict
from pathlib import Path
from dataclasses import dataclass
from collections import OrderedDict
import logging

import pcpp
from cxxheaderparser import types
from cxxheaderparser.simple import parse_string, ParsedData, NamespaceScope, ClassScope

from visp_python_bindgen.utils import *
from visp_python_bindgen.methods import *
from visp_python_bindgen.doc_parser import *
from visp_python_bindgen.header_utils import *
from visp_python_bindgen.generator_config import GeneratorConfig
from visp_python_bindgen.template_expansion import expand_templates


from typing import TYPE_CHECKING
if TYPE_CHECKING:
  from submodule import Submodule

class HeaderFile():
  def __init__(self, path: Path, submodule: 'Submodule'):
    self.path = path
    self.submodule = submodule
    self.includes = [f'<visp3/{self.submodule.name}/{self.path.name}>']
    self.binding_code = None
    self.header_repr = None
    self.contains = []
    self.depends = []
    self.documentation_holder_path: Path = None
    self.documentation_holder = None
    self.environment: HeaderEnvironment = None

  def __getstate__(self):
    return self.__dict__
  def __setstate__(self, d):
    self.__dict__ = d

  def get_header_dependencies(self, headers: List['HeaderFile']) -> List['HeaderFile']:
    if len(self.depends) == 0:
      return []

    user_required_headers = []
    all_user_required_header_names = self.submodule.config.get('header_additional_dependencies')
    if all_user_required_header_names is not None:
      user_required_headers = all_user_required_header_names.get(self.path.name, [])

    header_deps = []
    for header in headers:
      if header == self:
        continue
      is_dependency = False
      if header.path.name in user_required_headers:
        is_dependency = True
      else:
        for d in self.depends:
          if d in header.contains:
            is_dependency = True
            break
      if is_dependency:
        header_deps.append(header)
        upper_dependencies = header.get_header_dependencies(headers)
        header_deps.extend(upper_dependencies)
    return header_deps

  def preprocess(self) -> None:
    '''
    Preprocess the header to obtain the abstract representation of the cpp classes available.
    Additionally get the path to the xml documentation file generated by doxygen
    '''
    from cxxheaderparser.options import ParserOptions
    self.preprocessed_header_str = self.run_preprocessor() # Run preprocessor, get only code that can be compiled with current visp

    self.header_repr: ParsedData = parse_string(self.preprocessed_header_str, options=ParserOptions(verbose=False, convert_void_to_zero_params=True)) # Get the cxxheaderparser representation of the header

    # Get dependencies of this header. This is important for the code generation order
    for cls in self.header_repr.namespace.classes:
      name_cpp_no_template = '::'.join([seg.name for seg in cls.class_decl.typename.segments])
      self.contains.append(name_cpp_no_template)

      # Add parent classes as dependencies
      for base_class in cls.class_decl.bases:
        base_class_str_no_template = '::'.join([segment.name for segment in base_class.typename.segments])
        if base_class_str_no_template.startswith('vp'):
            self.depends.append(base_class_str_no_template)

      # Get documentation if available, only one document supported for now
      if self.documentation_holder_path is None:
        self.documentation_holder_path = DocumentationData.get_xml_path_if_exists(name_cpp_no_template, DocumentationObjectKind.Class)

  def run_preprocessor(self):
    logging.info(f'Preprocessing header {self.path.name}')
    tmp_dir = self.submodule.submodule_file_path.parent / "tmp"
    tmp_dir.mkdir(exist_ok=True)
    tmp_file_path = tmp_dir / (self.path.name + '.in')
    preprocessor_output_path = tmp_dir / (self.path.name)
    tmp_file_content = []

    # Includes that should be appended at the start of every file
    forced_includes = [
      'visp3/core/vpConfig.h', # Always include vpConfig: ensure that VISP macros are correctly defined
      'opencv2/opencv_modules.hpp'
    ]
    for include in forced_includes:
      tmp_file_content.append(f'#include <{include}>\n')

    # Remove all includes: we only include configuration headers, defined above
    with open(self.path.absolute(), 'r') as input_header_file:
      include_regex = '#include\s*<(.*)>'
      for line in input_header_file.readlines():
        matches = re.search(include_regex, line)
        if matches is None: # Include line if its not an include
          tmp_file_content.append(line)
        # else:
          # if 'visp3' in matches.group() or 'opencv' in matches.group():
          #   tmp_file_content.append(line)

    with open(tmp_file_path.absolute(), 'w') as tmp_file:
      tmp_file.write(''.join(tmp_file_content))
      tmp_file.flush()


    argv = [''] + GeneratorConfig.pcpp_config.to_pcpp_args_list()
    argv += ['-o', f'{preprocessor_output_path}', str(tmp_file_path.absolute())]
    argv_str = ", ".join(argv)
    logging.info(f'Preprocessor arguments:\n{argv_str}')

    pcpp.CmdPreprocessor(argv)
    preprocessed_header_content = None

    # Remove all #defines that could have been left by the preprocessor
    with open(preprocessor_output_path, 'r') as header_file:
      preprocessed_header_lines = []
      for line in header_file.readlines():
        if not line.startswith('#define'):
          preprocessed_header_lines.append(line)
      preprocessed_header_content = ''.join(preprocessed_header_lines)
      # Further refine header content: fix some simple parsing bugs
      preprocessed_header_content = preprocessed_header_content.replace('#include<', '#include <') # Bug in cpp header parser
      preprocessed_header_content = preprocessed_header_content.replace('inline friend', 'friend inline') # Bug in cpp header parser

    return preprocessed_header_content

  def generate_binding_code(self, bindings_container: BindingsContainer) -> None:
    assert self.header_repr is not None, 'The header was not preprocessed before calling the generation step!'
    self.parse_data(bindings_container)

  def compute_environment(self):
    '''
    Compute the header environment
    This environment contains:
      - The mapping from partially qualified names to fully qualified names
    If a class inherits from another, the environment should be updated with what is contained in the base class environment.
    This should be done in another step
    '''
    self.environment = HeaderEnvironment(self.header_repr)

  def parse_data(self, bindings_container: BindingsContainer) -> None:
    '''
    Update the bindings container passed in parameter with the bindings linked to this header file
    '''
    from visp_python_bindgen.enum_binding import get_enum_bindings
    # Fetch documentation if available
    if self.documentation_holder_path is not None:
      self.documentation_holder = DocumentationHolder(self.documentation_holder_path, self.environment.mapping)
    else:
      logging.warning(f'No documentation found for header {self.path}')

    for cls in self.header_repr.namespace.classes:
      self.generate_class(bindings_container, cls, self.environment)
    enum_bindings = get_enum_bindings(self.header_repr.namespace, self.environment.mapping, self.submodule, self)
    for enum_binding in enum_bindings:
      bindings_container.add_bindings(enum_binding)

    # Parse functions that are not linked to a class
    self.parse_sub_namespace(bindings_container, self.header_repr.namespace)

  def parse_sub_namespace(self, bindings_container: BindingsContainer, ns: NamespaceScope, namespace_prefix = '', is_root=True) -> None:
    '''
    Parse a subnamespace and all its subnamespaces.
    In a namespace, only the functions are exported.
    '''

    if not is_root and ns.name == '': # Anonymous namespace, only visible in header, so we ignore it
      return

    functions_with_configs, rejected_functions = get_bindable_functions_with_config(self.submodule, ns.functions, self.environment.mapping)

    # Log rejected functions
    rejection_strs = []
    for rejected_function in rejected_functions:
      self.submodule.report.add_non_generated_method(rejected_function)
      if NotGeneratedReason.is_non_trivial_reason(rejected_function.rejection_reason):
        rejection_strs.append(f'\t{rejected_function.signature} was rejected! Reason: {rejected_function.rejection_reason}')
    if len(rejection_strs) > 0:
      logging.warning(f'Rejected function in namespace: {ns.name}')
      logging.warning('\n' + '\n'.join(rejection_strs))

    bound_object = BoundObjectNames('submodule', self.submodule.name, namespace_prefix, namespace_prefix)
    defs = []
    for function, function_config in functions_with_configs:
      defs.append(define_method(function, function_config, False, {}, self, self.environment, bound_object)[0])

    bindings_container.add_bindings(SingleObjectBindings(bound_object, None, defs, GenerationObjectType.Namespace))
    for sub_ns in ns.namespaces:
      logging.info(f'Parsing subnamespace {namespace_prefix + sub_ns}')
      self.parse_sub_namespace(bindings_container, ns.namespaces[sub_ns], namespace_prefix + sub_ns + '::', False)

  def generate_class(self, bindings_container: BindingsContainer, cls: ClassScope, header_env: HeaderEnvironment) -> SingleObjectBindings:
    '''
    Generate the bindings for a single class:
    This method will generate one Python class per template instanciation.
    If the class has no template argument, then a single python class is generated

    If it is templated, the mapping (template argument types => Python class name) must be provided in the JSON config file
    '''
    def generate_class_with_potiental_specialization(name_python: str, owner_specs: 'OrderedDict[str, str]', cls_config: Dict) -> str:
      '''
      Generate the bindings of a single class, handling a potential template specialization.
      The handled information is:
        - The inheritance of this class
        - Its public fields that are not pointers
        - Its constructors
        - Most of its operators
        - Its public methods
      '''
      python_ident = f'py{name_python}'
      name_cpp = get_typename(cls.class_decl.typename, owner_specs, header_env.mapping)
      class_doc = None

      methods_dict: Dict[str, List[MethodBinding]] = {}
      def add_to_method_dict(key: str, value: MethodBinding):
        '''
        Add a method binding to the dictionary containing all the methods bindings of the class.
        This dict is a mapping str => List[MethodBinding]
        '''
        if key not in methods_dict:
          methods_dict[key] = [value]
        else:
          methods_dict[key].append(value)

      def add_method_doc_to_pyargs(method: types.Method, py_arg_strs: List[str]) -> List[str]:
        if self.documentation_holder is not None:
          method_name = get_name(method.name)
          method_doc_signature = MethodDocSignature(method_name,
                                                    get_type(method.return_type, {}, header_env.mapping) or '', # Don't use specializations so that we can match with doc
                                                    [get_type(param.type, {}, header_env.mapping) for param in method.parameters],
                                                    method.const, method.static)
          method_doc = self.documentation_holder.get_documentation_for_method(name_cpp_no_template, method_doc_signature, {}, owner_specs, param_names, [])
          if method_doc is None:
            logging.warning(f'Could not find documentation for {name_cpp}::{method_name}!')
            return py_arg_strs
          else:
            return [method_doc.documentation] + py_arg_strs
        else:
          return py_arg_strs

      if self.documentation_holder is not None:
        class_doc = self.documentation_holder.get_documentation_for_class(name_cpp_no_template, {}, owner_specs)
      else:
        logging.warning(f'Documentation not found when looking up {name_cpp_no_template}')

      # Declaration
      # Add template specializations to cpp class name. e.g., vpArray2D becomes vpArray2D<double> if the template T is double
      template_decl: Optional[types.TemplateDecl] = cls.class_decl.template
      if template_decl is not None:
        template_strs = []
        template_strs = map(lambda t: owner_specs[t.name], template_decl.params)
        template_str = f'<{", ".join(template_strs)}>'
        name_cpp += template_str

      # Reference public base classes when creating pybind class binding
      base_class_strs = list(map(lambda base_class: get_typename(base_class.typename, owner_specs, header_env.mapping),
                            filter(lambda b: b.access == 'public', cls.class_decl.bases)))
      # py::class template contains the class, its holder type, and its base clases.
      # The default holder type is std::unique_ptr. when the cpp function argument is a shared_ptr, Pybind will raise an error when calling the method.
      py_class_template_str = ', '.join([name_cpp, f'std::shared_ptr<{name_cpp}>'] + base_class_strs)
      doc_param = [] if class_doc is None else [class_doc.documentation]
      buffer_protocol_arg = ['py::buffer_protocol()'] if cls_config['use_buffer_protocol'] else []
      cls_argument_strs = ['submodule', f'"{name_python}"'] + doc_param + buffer_protocol_arg

      class_decl = f'\tpy::class_ {python_ident} = py::class_<{py_class_template_str}>({", ".join(cls_argument_strs)});'

      # Definitions
      # Skip constructors for classes that have pure virtual methods since they cannot be instantiated
      contains_pure_virtual_methods = False
      for method in cls.methods:
        if method.pure_virtual:
          contains_pure_virtual_methods = True
          break

      # User marked this class as virtual.
      # This is required if no virtual method is declared in this class,
      #  but it does not implement pure virtual methods of a base class
      contains_pure_virtual_methods = contains_pure_virtual_methods or cls_config['is_virtual']

      # Find bindable methods
      generated_methods: List[MethodData] = []
      bindable_methods_and_config, rejected_methods = get_bindable_methods_with_config(self.submodule, cls.methods,
                                                                                    name_cpp_no_template, owner_specs, header_env.mapping)
      # Display rejected methods
      rejection_strs = []
      for rejected_method in rejected_methods:
        self.submodule.report.add_non_generated_method(rejected_method)
        if NotGeneratedReason.is_non_trivial_reason(rejected_method.rejection_reason):
          rejection_strs.append(f'\t{rejected_method.signature} was rejected! Reason: {rejected_method.rejection_reason}')
      if len(rejection_strs) > 0:
        logging.warning(f'Rejected method in class: {name_cpp}')
        logging.warning('\n'.join(rejection_strs))

      # Split between constructors and other methods
      constructors, non_constructors = split_methods_with_config(bindable_methods_and_config, lambda m: m.constructor)

      # Split between "normal" methods and operators, which require a specific definition
      cpp_operator_names = cpp_operator_list()
      operators, basic_methods = split_methods_with_config(non_constructors, lambda m: get_name(m.name) in cpp_operator_names)

      # Constructors definitions
      if not contains_pure_virtual_methods:
        for method, method_config in constructors:
          method_name = get_name(method.name)
          params_strs = [get_type(param.type, owner_specs, header_env.mapping) for param in method.parameters]
          py_arg_strs = get_py_args(method.parameters, owner_specs, header_env.mapping)
          param_names = [param.name or 'arg' + str(i) for i, param in enumerate(method.parameters)]

          py_arg_strs = add_method_doc_to_pyargs(method, py_arg_strs)

          ctor_str = f'''{python_ident}.{define_constructor(params_strs, py_arg_strs)};'''
          add_to_method_dict('__init__', MethodBinding(ctor_str, is_static=False, is_lambda=False,
                                                       is_operator=False, is_constructor=True))

      # Operator definitions

      for method, method_config in operators:
        method_name = get_name(method.name)
        method_is_const = method.const
        params_strs = [get_type(param.type, owner_specs, header_env.mapping) for param in method.parameters]
        return_type_str = get_type(method.return_type, owner_specs, header_env.mapping)
        py_args = get_py_args(method.parameters, owner_specs, header_env.mapping)
        py_args = py_args + ['py::is_operator()']
        param_names = [param.name or 'arg' + str(i) for i, param in enumerate(method.parameters)]
        py_args = add_method_doc_to_pyargs(method, py_args)

        # if len(params_strs) > 1:
        #   logging.info(f'Found operator {name_cpp}{method_name} with more than one parameter, skipping')
        #   rejection_param_strs = [get_type(param.type, {}, header_env.mapping) for param in method.parameters]
        #   rejection_return_type_str = get_type(method.return_type, {}, header_env.mapping)
        #   rejection = RejectedMethod(name_cpp, method, method_config, get_method_signature(method_name, rejection_return_type_str, rejection_param_strs), NotGeneratedReason.NotHandled)
        #   self.submodule.report.add_non_generated_method(rejection)
        #   continue
        if len(params_strs) < 1: # Unary ops
          for cpp_op, python_op_name in supported_const_return_unary_op_map().items():
            if method_name == f'operator{cpp_op}':
              operator_str = lambda_const_return_unary_op(python_ident, python_op_name, cpp_op,
                                                         method_is_const, name_cpp,
                                                         return_type_str, py_args)
              add_to_method_dict(f'__{python_op_name}__', MethodBinding(operator_str, is_static=False, is_lambda=True,
                                                       is_operator=True, is_constructor=False))
              break
        elif len(params_strs) == 1: # e.g., self + other
          for cpp_op, python_op_name in supported_const_return_binary_op_map().items():
            if method_name == f'operator{cpp_op}':
              operator_str = lambda_const_return_binary_op(python_ident, python_op_name, cpp_op,
                                                          method_is_const, name_cpp, params_strs[0],
                                                          return_type_str, py_args)
              add_to_method_dict(f'__{python_op_name}__', MethodBinding(operator_str, is_static=False, is_lambda=True,
                                                        is_operator=True, is_constructor=False))
              break
          for cpp_op, python_op_name in supported_in_place_binary_op_map().items():
            if method_name == f'operator{cpp_op}':
              operator_str = lambda_in_place_binary_op(python_ident, python_op_name, cpp_op,
                                                      method_is_const, name_cpp, params_strs[0],
                                                      return_type_str, py_args)
              add_to_method_dict(f'__{python_op_name}__', MethodBinding(operator_str, is_static=False, is_lambda=True,
                                                        is_operator=True, is_constructor=False))
              break
        else: # N-ary operators
          for cpp_op, python_op_name in supported_nary_op_map().items():
            if method_name == f'operator{cpp_op}':
              operator_str = lambda_nary_op(python_ident, python_op_name, cpp_op,
                                                    method_is_const, name_cpp, params_strs,
                                                    return_type_str, py_args)
              add_to_method_dict(f'__{python_op_name}__', MethodBinding(operator_str, is_static=False, is_lambda=True,
                                                        is_operator=True, is_constructor=False))
              break

      # Define classical methods
      class_def_names = BoundObjectNames(python_ident, name_python, name_cpp_no_template, name_cpp)
      for method, method_config in basic_methods:
        if method.template is not None and method_config.get('specializations') is not None:
          method_template_names = [t.name for t in method.template.params]
          specializations = method_config.get('specializations')
          specializations = expand_templates(specializations)
          for method_spec in specializations:
            new_specs = owner_specs.copy()
            assert len(method_template_names) == len(method_spec)
            method_spec_dict = OrderedDict(k for k in zip(method_template_names, method_spec))
            new_specs.update(method_spec_dict)
            method_str, method_data = define_method(method, method_config, True,
                                                                new_specs, self, header_env, class_def_names)
            add_to_method_dict(method_data.py_name, MethodBinding(method_str, is_static=method.static,
                                                                  is_lambda=f'{name_cpp}::*' not in method_str,
                                                                  is_operator=False, is_constructor=False,
                                                                  method_data=method_data))
            generated_methods.append(method_data)
        else:
          method_str, method_data = define_method(method, method_config, True,
                                                              owner_specs, self, header_env, class_def_names)
          add_to_method_dict(method_data.py_name, MethodBinding(method_str, is_static=method.static,
                                                                is_lambda=f'{name_cpp}::*' not in method_str,
                                                                is_operator=False, is_constructor=False,
                                                                method_data=method_data))
          generated_methods.append(method_data)

      # See https://github.com/pybind/pybind11/issues/974
      # Update with overloads that are shadowed by new overloads defined in this class
      # For instance, declaring:
      # class A { void foo(int); };
      # class B: public A { void foo(std::string& s); }
      # Will result in the following code generating an error:
      # from visp.core import B
      # b = B()
      # b.foo(0) # no overload known with int
      base_bindings = list(filter(lambda b: b is not None, map(lambda s: bindings_container.find_bindings(s), base_class_strs)))

      # assert not any(map(lambda b: b is None, base_bindings)), f'Could not retrieve the bindings for a base class of {name_cpp}'
      for base_binding_container in base_bindings:
        base_defs = base_binding_container.definitions
        if not isinstance(base_defs, ClassBindingDefinitions):
          raise RuntimeError
        base_methods_dict = base_defs.methods
        for method_name in methods_dict.keys():
          if method_name == '__init__': # Do not bring constructors of the base class in this class defs as it makes no sense
            continue
          if method_name in base_methods_dict:
            for parent_method_binding in base_methods_dict[method_name]:
              methods_dict[method_name].append(parent_method_binding.get_definition_in_child_class(python_ident))

      # Add to string representation
      if not cls_config['ignore_repr']:
        to_string_str = find_and_define_repr_str(cls, name_cpp, python_ident)
        if len(to_string_str) > 0:
          add_to_method_dict('__repr__', MethodBinding(to_string_str, is_static=False, is_lambda=True, is_operator=True,
                                                       is_constructor=False))

      # Add call to user defined bindings function
      # Binding function should be defined in the static part of the generator
      # It should have the signature void fn(py::class_<Type>& cls);
      # If it is for a templated class, it should also be templated in the same way (same order of parameters etc.)
      if cls_config['additional_bindings'] is not None:
        template_str = ''
        if len(owner_specs.keys()) > 0:
          template_types = owner_specs.values()
          template_str = f'<{", ".join([template_type for template_type in template_types])}>'
        add_to_method_dict('__additional_bindings', MethodBinding(f'{cls_config["additional_bindings"]}({python_ident});',
                                                                  is_static=False, is_lambda=False,
                                                                  is_operator=False, is_constructor=False))

      # Check for potential error-generating definitions
      error_generating_overloads = get_static_and_instance_overloads(generated_methods)
      if len(error_generating_overloads) > 0:
        logging.error(f'Overloads defined for instance and class, this will generate a pybind error')
        logging.error(error_generating_overloads)
        raise RuntimeError('Error generating overloads:\n' + '\n'.join(error_generating_overloads))

      field_dict = {}
      for field in cls.fields:
        if field.name in cls_config['ignored_attributes']:
          logging.info(f'Ignoring field in class/struct {name_cpp}: {field.name}')
          continue
        if field.access == 'public':
          if is_unsupported_argument_type(field.type):
            continue

          field_type = get_type(field.type, owner_specs, header_env.mapping)
          logging.info(f'Found field in class/struct {name_cpp}: {field_type} {field.name}')

          field_name_python = field.name.lstrip('m_')

          def_str = 'def_'
          def_str += 'readonly' if field.type.const else 'readwrite'
          if field.static:
            def_str += '_static'

          field_str = f'{python_ident}.{def_str}("{field_name_python}", &{name_cpp}::{field.name});'
          field_dict[field_name_python] = field_str

      classs_binding_defs = ClassBindingDefinitions(field_dict, methods_dict)
      bindings_container.add_bindings(SingleObjectBindings(class_def_names, class_decl, classs_binding_defs, GenerationObjectType.Class))

    name_cpp_no_template = '::'.join([seg.name for seg in cls.class_decl.typename.segments])
    logging.info(f'Parsing class "{name_cpp_no_template}"')

    if self.submodule.class_should_be_ignored(name_cpp_no_template):
      return ''

    cls_config = self.submodule.get_class_config(name_cpp_no_template)

    # Warning for potential double frees
    acknowledged_pointer_fields = cls_config.get('acknowledge_pointer_or_ref_fields') or []
    refs_or_ptr_fields = list(filter(lambda tn: '&' in tn or '*' in tn,
                                map(lambda field: get_type(field.type, {}, header_env.mapping),
                                    cls.fields)))

    # If some pointer or refs are not acknowledged as existing by user, emit a warning
    if len(set(refs_or_ptr_fields).difference(set(acknowledged_pointer_fields))) > 0:
      self.submodule.report.add_pointer_or_ref_holder(name_cpp_no_template, refs_or_ptr_fields)


    if cls.class_decl.template is None:
      name_python = name_cpp_no_template.replace('vp', '')
      return generate_class_with_potiental_specialization(name_python, {}, cls_config)
    else:
      if cls_config is None or 'specializations' not in cls_config or len(cls_config['specializations']) == 0:
        logging.warning(f'Could not find template specialization for class {name_cpp_no_template}: skipping!')
        self.submodule.report.add_non_generated_class(name_cpp_no_template, cls_config, 'Skipped because there was no declared specializations')
      else:
        specs = cls_config['specializations']
        template_names = [t.name for t in cls.class_decl.template.params]
        for spec in specs:
          name_python = spec['python_name']
          args = spec['arguments']
          assert len(template_names) == len(args), f'Specializing {name_cpp_no_template}: Template arguments are {template_names} but found specialization {args} which has the wrong number of arguments'
          spec_dict = OrderedDict(k for k in zip(template_names, args))
          generate_class_with_potiental_specialization(name_python, spec_dict, cls_config)
