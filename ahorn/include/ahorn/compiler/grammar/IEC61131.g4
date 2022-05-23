grammar IEC61131;

sourceCode: (dataTypeDeclaration | functionBlockDeclaration | functionDeclaration)* programDeclaration?
(dataTypeDeclaration | functionBlockDeclaration | functionDeclaration)* EOF;

programDeclaration:
    PROGRAM programName
        interface
        functionBlockBody
    END_PROGRAM;

programName: IDENTIFIER;

functionBlockDeclaration:
    FUNCTION_BLOCK derivedFunctionBlockName
        interface
        functionBlockBody
    END_FUNCTION_BLOCK;

functionBlockBody: sequentialFunctionChart | statementList;

functionDeclaration:
    FUNCTION functionName
        interface
        functionBody
    END_FUNCTION;

functionName: IDENTIFIER;

functionBody: statementList;

interface: (inputVariableDeclarations | localVariableDeclarations | outputVariableDeclarations | temporaryVariableDeclarations)*;

inputVariableDeclarations:
    VAR_INPUT
        (variableDeclarationInitialization[true, false, false, false])*
    END_VAR;

localVariableDeclarations:
    VAR
        (variableDeclarationInitialization[false, true, false, false])*
    END_VAR;

outputVariableDeclarations:
    VAR_OUTPUT
        (variableDeclarationInitialization[false, false, true, false])*
    END_VAR;

temporaryVariableDeclarations:
    VAR_TEMP
        (variableDeclarationInitialization[false, false, false, true])*
    END_VAR;

variableDeclarationInitialization[bool input, bool local, bool output, bool temporary]:
(variableList COLON (simpleSpecificationInitialization | enumeratedSpecificationInitialization)
| structureVariableDeclarationInitialization
| functionBlockDeclarationInitialization) SEMICOLON;

variableList: variableName (COMMA variableName)*;

structureVariableDeclarationInitialization: variableList COLON structureSpecificationInitialization;

structureSpecificationInitialization: structureTypeName (ASSIGNMENT structureInitialization)?;

structureTypeName: IDENTIFIER;

simpleSpecificationInitialization: simpleSpecification (ASSIGNMENT constant)?;

simpleSpecification: elementaryTypeName | safetyTypeName | simpleTypeName;

simpleTypeName: IDENTIFIER;

functionBlockDeclarationInitialization: functionBlockNameList COLON functionBlockTypeName (ASSIGNMENT
structureInitialization)?;

functionBlockNameList: functionBlockName (COMMA functionBlockName)*;

functionBlockName: IDENTIFIER;

functionBlockTypeName: standardFunctionBlockName | derivedFunctionBlockName;

standardFunctionBlockName: 'SR' | 'RS'; // | 'R_TRIG' | 'F_TRIG' | 'CTU' | 'CTD' | 'CTUD' | 'TP' | 'TON' | 'TOF';

derivedFunctionBlockName: IDENTIFIER;

structureInitialization: LEFT_PARENTHESIS structureElementInitialization (COMMA structureElementInitialization)*
RIGHT_PARENTHESIS;

structureElementInitialization: structureElementName ASSIGNMENT (constant | enumeratedValue | structureInitialization);

structureElementName: IDENTIFIER;

variableName: IDENTIFIER;

dataTypeDeclaration: TYPE (typeDeclaration SEMICOLON)+ END_TYPE;

typeDeclaration: enumeratedTypeDeclaration;

enumeratedTypeDeclaration: enumeratedTypeName COLON enumeratedSpecificationInitialization;

enumeratedTypeName: IDENTIFIER;

enumeratedSpecificationInitialization: enumeratedSpecification (ASSIGNMENT enumeratedValue)?;

enumeratedSpecification: (LEFT_PARENTHESIS values+=enumeratedValueSpecification (COMMA
values+=enumeratedValueSpecification)* RIGHT_PARENTHESIS) | enumeratedTypeName;

enumeratedValueSpecification: IDENTIFIER;

enumeratedValue: (enumeratedTypeName '#')? IDENTIFIER;

elementaryTypeName: numericTypeName | bitStringTypeName | booleanTypeName | timeTypeName;

numericTypeName: integerTypeName | realTypeName;

integerTypeName: signedIntegerTypeName | unsignedIntegerTypeName;

unsignedIntegerTypeName: USINT | UINT | UDINT | ULINT;

signedIntegerTypeName: SINT | INT | DINT | LINT;

realTypeName: REAL | LREAL;

bitStringTypeName: BYTE | WORD;

booleanTypeName: BOOL;

timeTypeName: TIME;

derivedTypeName: IDENTIFIER;

safetyTypeName: safetyBooleanTypeName;

safetyBooleanTypeName: SAFEBOOL;

statement: assignmentStatement | assumeStatement | assertStatement | subprogramControlStatement | selectionStatement
| iterationStatement;

assignmentStatement: variableReference ASSIGNMENT (expression | nondeterministicConstant);

nondeterministicConstant: (NONDETERMINISTIC_BOOL | NONDETERMINISTIC_INT | NONDETERMINISTIC_TIME) LEFT_PARENTHESIS RIGHT_PARENTHESIS;

assumeStatement: ASSUME LEFT_PARENTHESIS expression RIGHT_PARENTHESIS;

assertStatement: ASSERT LEFT_PARENTHESIS expression RIGHT_PARENTHESIS;

subprogramControlStatement: invocationStatement;

invocationStatement: (functionBlockInstanceName | functionBlockInstanceName DOT variableReference) LEFT_PARENTHESIS
(parameterAssignment
(COMMA parameterAssignment)*)? RIGHT_PARENTHESIS;

functionBlockInstanceName: IDENTIFIER;

parameterAssignment locals[bool pre_assignment, bool post_assignment]:
    ({$pre_assignment = true; $post_assignment = false;} (calleeInputVariable=variableName ASSIGNMENT)? expression)
    | ({$pre_assignment = false; $post_assignment = true;} calleeOutputVariable=variableName ARROW_RIGHT
    callerVariable=variableName);

selectionStatement: ifStatement | caseStatement;

ifStatement:
    IF ifCondition=expression THEN
        (ifStatementList=statementList)
    (ELSIF elseIfConditions+=expression THEN
        elseIfStatementLists+=statementList)*
    (ELSE
        (elseStatementList=statementList)
    )?
    END_IF;

statementList: (statement? SEMICOLON)*;

caseStatement:
    CASE expression OF
        caseSelection+
    (ELSE
        statementList)?
    END_CASE;

caseSelection: caseList COLON statementList;

caseList: caseListElement (COMMA caseListElement)*;

caseListElement: expression | enumeratedValue;

iterationStatement: whileStatement;

whileStatement:
    WHILE expression DO
        statementList
    END_WHILE;

expression:
    <assoc=right> unaryOperator=(MINUS | PLUS | COMPLEMENT)? primaryExpression                                                                    #unaryExpression
    | leftOperand=expression binaryOperator=EXPONENTIATION rightOperand=expression                                                                #binaryExpression
    | <assoc=right> leftOperand=expression binaryOperator=(DIVIDE | MODULO) rightOperand=expression                                               #binaryExpression
    | leftOperand=expression binaryOperator=MULTIPLY rightOperand=expression                                                                      #binaryExpression
    | leftOperand=expression binaryOperator=(PLUS | MINUS) rightOperand=expression                                                                #binaryExpression
    | leftOperand=expression binaryOperator=(LESS_THAN | GREATER_THAN | LESS_THAN_OR_EQUAL_TO | GREATER_THAN_OR_EQUAL_TO) rightOperand=expression #binaryExpression
    | leftOperand=expression binaryOperator=(EQUALITY | INEQUALITY) rightOperand=expression                                                       #binaryExpression
    | leftOperand=expression binaryOperator=BOOLEAN_AND rightOperand=expression                                                                   #binaryExpression
    | leftOperand=expression binaryOperator=BOOLEAN_EXCLUSIVE_OR rightOperand=expression                                                          #binaryExpression
    | leftOperand=expression binaryOperator=BOOLEAN_OR rightOperand=expression                                                                    #binaryExpression
    | CHANGE LEFT_PARENTHESIS oldOperand=expression COMMA newOperand=expression RIGHT_PARENTHESIS                                                 #changeExpression
    ;

primaryExpression: constant | enumeratedValue | variableReference | functionCall | LEFT_PARENTHESIS expression
RIGHT_PARENTHESIS;

functionCall: functionName LEFT_PARENTHESIS (parameterAssignment (COMMA parameterAssignment)*)? RIGHT_PARENTHESIS;

constant: numericLiteral | timeLiteral | booleanLiteral;

variableReference: variableAccess | fieldAccess;

variableAccess : variableName;

fieldAccess : recordAccess DOT variableReference;

recordAccess : variableAccess;

numericLiteral: integerLiteral;

integerLiteral: (integerTypeName NUMBER_SIGN)? signedInteger | HEXADECIMAL_INTEGER;

signedInteger: (PLUS | MINUS)? unsignedInteger;

unsignedInteger: UNSIGNED_INTEGER;

timeLiteral: (timeTypeName | 'T' | 'LT') '#' ('+' | '-')? INTERVAL;

booleanLiteral: (booleanTypeName NUMBER_SIGN)? (FALSE | TRUE);

// Sequential Function Chart (SFC)
sequentialFunctionChart: sequentialFunctionChartNetwork+;

sequentialFunctionChartNetwork: initialStep (step | transition | action)*;

initialStep: INITIAL_STEP stepName COLON (actionAssociation SEMICOLON)* END_STEP;

step: STEP stepName COLON (actionAssociation SEMICOLON)* END_STEP;

stepName: IDENTIFIER;

actionAssociation: actionName LEFT_PARENTHESIS actionQualifier? (COMMA indicatorName)* RIGHT_PARENTHESIS;

actionName: IDENTIFIER;

actionQualifier: IDENTIFIER COMMA actionTime; // 'N' | 'R' | 'S' | 'P' | timedQualifier COMMA actionTime;

timedQualifier: IDENTIFIER; // 'L' | 'D' | 'SD' | 'DS' | 'SL';

actionTime: INTERVAL | variableName;

indicatorName: variableName;

transition:
    TRANSITION transitionName? (LEFT_PARENTHESIS PRIORITY ASSIGNMENT unsignedInteger RIGHT_PARENTHESIS)?
    FROM steps TO steps transitionCondition
    END_TRANSITION;

transitionName: IDENTIFIER;

steps: stepName | LEFT_PARENTHESIS stepName (COMMA stepName)+ RIGHT_PARENTHESIS;

transitionCondition: ASSIGNMENT expression SEMICOLON;

action: ACTION actionName COLON functionBlockBody END_ACTION;

fragment A : ('a'|'A');
fragment B : ('b'|'B');
fragment C : ('c'|'C');
fragment D : ('d'|'D');
fragment E : ('e'|'E');
fragment F : ('f'|'F');
fragment G : ('g'|'G');
fragment H : ('h'|'H');
fragment I : ('i'|'I');
fragment J : ('j'|'J');
fragment K : ('k'|'K');
fragment L : ('l'|'L');
fragment M : ('m'|'M');
fragment N : ('n'|'N');
fragment O : ('o'|'O');
fragment P : ('p'|'P');
fragment Q : ('q'|'Q');
fragment R : ('r'|'R');
fragment S : ('s'|'S');
fragment T : ('t'|'T');
fragment U : ('u'|'U');
fragment V : ('v'|'V');
fragment W : ('w'|'W');
fragment X : ('x'|'X');
fragment Y : ('y'|'Y');
fragment Z : ('z'|'Z');

NUMBER_SIGN: '#';
MINUS: '-';
PLUS: '+';
ARROW_RIGHT: '=>';
ASSIGNMENT: ':=';
COMMA: ',';
COLON: ':';
DOT: '.';
SEMICOLON: ';';

// Operators
// Precedence: 11 (Highest)
LEFT_PARENTHESIS: '(';
RIGHT_PARENTHESIS: ')';
// Precedence 9
DEREFERENCE: '^';
// Precedence: 8
// NEGATION: '-';
// UNARY_PLUS: '+';
COMPLEMENT : N O T;
// Precedence: 7
EXPONENTIATION : '**';
// Precedence: 6
MULTIPLY: '*';
DIVIDE : '/';
MODULO : M O D;
// Precedence: 5
// ADD: '+';
// SUBTRACT: '-';
// Precedence: 4
LESS_THAN : '<';
GREATER_THAN : '>';
LESS_THAN_OR_EQUAL_TO : '<=';
GREATER_THAN_OR_EQUAL_TO : '>=';
EQUALITY : '=';
INEQUALITY : '<>';
// Precedence: 3
BOOLEAN_AND : '&' | A N D;
// Precedence: 2
BOOLEAN_EXCLUSIVE_OR : X O R;
// Precedence: 1 (Lowest)
BOOLEAN_OR : O R;

ASSUME : A S S U M E;
ASSERT : A S S E R T;
CHANGE : C H A N G E;

IF : I F;
THEN : T H E N;
ELSIF : E L S IF;
ELSE : E L S E;
END_IF : E N D '_' IF;

CASE : C A S E;
OF : O F;
END_CASE : E N D '_' CASE;

WHILE : W H I L E;
DO : D O;
END_WHILE : E N D '_' WHILE;

TYPE : T Y P E;
END_TYPE : E N D '_' TYPE;

// Sequential Function Chart (SFC)
STEP : S T E P;
INITIAL_STEP : I N I T I A L '_' STEP;
END_STEP : E N D '_' STEP;
TRANSITION: T R A N S I T I O N;
PRIORITY : P R I O R I T Y;
FROM : F R O M;
TO : T O;
END_TRANSITION: E N D '_' TRANSITION;
ACTION: A C T I O N;
END_ACTION: E N D '_' ACTION;

INT : I N T;
SINT : S INT;
DINT : D INT;
LINT : L INT;
UINT : U INT;
USINT : U SINT;
UDINT : U DINT;
ULINT : U LINT;

REAL : R E A L;
LREAL : L REAL;

BYTE: B Y T E;
WORD: W O R D;

BOOL : B O O L;
SAFEBOOL : S A F E BOOL;

TIME: T I M E;

FALSE : F A L S E;
TRUE : T R U E;

NONDETERMINISTIC_BOOL : N O N D E T E R M I N I S T I C '_' BOOL;
NONDETERMINISTIC_INT : N O N D E T E R M I N I S T I C '_' INT;
NONDETERMINISTIC_TIME : N O N D E T E R M I N I S T I C '_' TIME;

VAR: V A R;
VAR_INPUT: VAR '_' I N P U T;
VAR_OUTPUT: VAR '_' O U T P U T;
VAR_TEMP: VAR '_' T E M P;
END_VAR: E N D '_' VAR;

PROGRAM: P R O G R A M;
END_PROGRAM: E N D '_' PROGRAM;

FUNCTION_BLOCK: F U N C T I O N '_' B L O C K;
END_FUNCTION_BLOCK: E N D '_' FUNCTION_BLOCK;

FUNCTION: F U N C T I O N;
END_FUNCTION: E N D '_' F U N C T I O N;

fragment DIGIT : '0' .. '9';
fragment HEXADECIMAL_DIGIT : DIGIT | 'A' .. 'F';
UNSIGNED_INTEGER : DIGIT ('_'? DIGIT)*;
HEXADECIMAL_INTEGER : '16#' HEXADECIMAL_DIGIT+;

fragment FIXED_POINT : UNSIGNED_INTEGER (DOT UNSIGNED_INTEGER)?;
INTERVAL : DAYS | HOURS | MINUTES | SECONDS | MILLISECONDS | MICROSECONDS | NANOSECONDS;
fragment DAYS : (FIXED_POINT 'd') | (UNSIGNED_INTEGER 'd' '_'?) HOURS?;
fragment HOURS : (FIXED_POINT 'h') | (UNSIGNED_INTEGER 'h' '_'?) MINUTES?;
fragment MINUTES : (FIXED_POINT 'm') | (UNSIGNED_INTEGER 'm' '_'?) SECONDS?;
fragment SECONDS : (FIXED_POINT 's') | (UNSIGNED_INTEGER 's' '_'?) MILLISECONDS?;
fragment MILLISECONDS : (FIXED_POINT 'ms') | (UNSIGNED_INTEGER 'ms' '_'?) MICROSECONDS?;
fragment MICROSECONDS : (FIXED_POINT 'us') | (UNSIGNED_INTEGER 'us' '_'?) NANOSECONDS?;
fragment NANOSECONDS : FIXED_POINT 'ns';

IDENTIFIER : [A-Za-z][A-Za-z_0-9]*;

COMMENT : ('(*' .*? '*)' | '/*' .*? '*/') -> channel (HIDDEN);
LINE_COMMENT : '//' ~[\r\n]* -> channel(HIDDEN);
WHITE_SPACE : [ \n\r\t]+ -> channel(HIDDEN);
EOL : '\n';