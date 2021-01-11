module ZedBundled = Oni_Core.ZedBundled;
[@deriving show]
type t =
  | PhysicalKey(PhysicalKey.t)
  | SpecialKey(SpecialKey.t);

let physicalKey = (~key, ~modifiers) =>
  PhysicalKey(PhysicalKey.{key, modifiers});

let specialKey = special => SpecialKey(special);

let toPhysicalKey =
  fun
  | PhysicalKey(key) => Some(key)
  | SpecialKey(_) => None;

let equals = (keyA, keyB) => {
  switch (keyA, keyB) {
  | (SpecialKey(specialKeyA), SpecialKey(specialKeyB)) =>
    specialKeyA == specialKeyB
  | (PhysicalKey(physicalKeyA), PhysicalKey(physicalKeyB)) =>
    physicalKeyA.key == physicalKeyB.key
    && Modifiers.equals(physicalKeyA.modifiers, physicalKeyB.modifiers)
  | (SpecialKey(_), PhysicalKey(_))
  | (PhysicalKey(_), SpecialKey(_)) => false
  };
};

let ofInternal =
    (
      ~addShiftKeyToCapital,
      (
        key: Matcher_internal.keyPress,
        mods: list(Matcher_internal.modifier),
      ),
    ) => {
  let keyToKeyPress = (~mods=mods, key) => {
    Ok(
      PhysicalKey({
        key,
        modifiers: Matcher_internal.Helpers.internalModsToMods(mods),
      }),
    );
  };
  switch (key) {
  | Matcher_internal.UnmatchedString(str) =>
  prerr_endline ("--str: " ++ str);
    ZedBundled.explode(str)
    |> List.map(uchar =>
         if (Uchar.is_char(uchar)) {
           prerr_endline ("NOt uchar")
           let char = Uchar.to_char(uchar);
           let lowercaseChar = Char.lowercase_ascii(char);
           let isCapitalized = lowercaseChar != char;
           if (isCapitalized && addShiftKeyToCapital) {
             keyToKeyPress(
               ~mods=[Shift, ...mods],
               Key.Character(lowercaseChar),
             );
           } else {
             keyToKeyPress(Key.Character(lowercaseChar));
           };
         } else {
           prerr_endline ("Unicode charecters not yet supported in bindings: " ++ ZedBundled.make(1, uchar));
           keyToKeyPress(Key.Character('o'));
           // Error(
           //   "Unicode characters not yet supported in bindings: "
           //   ++ ZedBundled.make(1, uchar),
           // );
         }
       )
  | Matcher_internal.Special(special) => [Ok(SpecialKey(special))]
  | Matcher_internal.Physical(key) => [keyToKeyPress(key)]
  };
};

let parse = (~explicitShiftKeyNeeded, str) => {
  let parse = lexbuf =>
    switch (Matcher_parser.keys(Matcher_lexer.token, lexbuf)) {
    | exception Matcher_lexer.Error => Error("Error parsing binding: " ++ str)
    | exception (Matcher_lexer.UnrecognizedModifier(m)) =>
      Error("Unrecognized modifier:" ++ m ++ " in: " ++ str)
    | exception Matcher_parser.Error =>
      Error("Error parsing binding: " ++ str)
    | v => Ok(v)
    };

  prerr_endline ("Parsing str: " ++ str);
  let flatMap = (f, r) => Result.bind(r, f);

  let addShiftKeyToCapital = !explicitShiftKeyNeeded;

  let finish = r => {
    r
    |> List.map(ofInternal(~addShiftKeyToCapital))
    |> List.flatten
    |> Base.Result.all;
  };

  str |> Lexing.from_string |> parse |> flatMap(finish);
};

let toString = (~meta="Meta", ~keyToString=Key.toString, key) => {
  switch (key) {
  | SpecialKey(special) =>
    Printf.sprintf("Special(%s)", SpecialKey.show(special))
  | PhysicalKey({key, modifiers, _}) =>
    let buffer = Buffer.create(16);
    let separator = "+";

    let keyString = keyToString(key);

    let onlyShiftPressed =
      modifiers.shift
      && !modifiers.control
      && !modifiers.meta
      && !modifiers.alt;

    let keyString =
      String.length(keyString) == 1 && !onlyShiftPressed
        ? String.lowercase_ascii(keyString) : keyString;

    if (modifiers.meta) {
      Buffer.add_string(buffer, meta ++ separator);
    };

    if (modifiers.control) {
      Buffer.add_string(buffer, "Ctrl" ++ separator);
    };

    if (modifiers.altGr) {
      Buffer.add_string(buffer, "AltGr" ++ separator);
    } else if (modifiers.alt) {
      Buffer.add_string(buffer, "Alt" ++ separator);
    };

    if ((modifiers.meta || modifiers.control || modifiers.alt)
        && modifiers.shift) {
      Buffer.add_string(buffer, "Shift" ++ separator);
    };

    Buffer.add_string(buffer, keyString);

    Buffer.contents(buffer);
  };
};
